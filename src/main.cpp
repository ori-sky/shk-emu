#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <vector>

#include <shk.h>

struct cpu_state {
	uint16_t mem[0x10000] = {0};
	uint16_t reg[0x100] = {0};
	uint16_t ip = 0xFF;
};

void debug(cpu_state &state) {
	for(size_t i = 0x0; i < 0x8; ++i) {
		std::cout << "mem[";
		std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << i;
		std::cout << "] = ";
		std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << state.mem[i];
		std::cout << "   ";
		std::cout << "reg[";
		std::cout << "0x" << std::setfill('0') << std::setw(2) << std::hex << i;
		std::cout << "] = ";
		std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << state.reg[i];
		std::cout << std::endl;
	}
	std::cout << "ip = ";
	std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << state.reg[state.ip];
	std::cout << std::endl;
}

std::optional<shk::instruction> decode(cpu_state &state) {
	shk::instruction instr;

	auto byte = state.mem[state.reg[state.ip]++];

	if(byte >> 15u) {
		shk::command command;
		command.ty = static_cast<shk::command::type>(byte & 0xFF);

		for(size_t i = 0; i < shk::num_operands(command.ty); ++i) {
			auto byte = state.mem[state.reg[state.ip]++];

			shk::operand operand;
			operand.ty = static_cast<shk::operand::type>(byte >> 15u);
			operand.value = byte & 0xFF;
			command.operands.emplace_back(operand);
		}

		auto ins = decode(state);
		if(!ins) {
			return {};
		}

		instr = *ins;
		instr.commands.emplace_back(command);
	} else {
		instr.op = static_cast<shk::opcode>(byte);
		for(size_t i = 0; i < shk::num_operands(instr.op); ++i) {
			auto byte = state.mem[state.reg[state.ip]++];

			shk::operand operand;
			operand.ty = static_cast<shk::operand::type>(byte >> 15u);
			operand.value = byte & 0xFF;
			instr.operands.emplace_back(operand);
		}
	}

	std::cout << "decoded " << instr.op << std::endl;
	return instr;
}

uint16_t eval_ref(cpu_state &state, shk::operand &operand) {
	switch(operand.ty) {
	case shk::operand::type::imm:
		std::cerr << "error: eval_ref: cannot reference an immediate" << std::endl;
		return 0;
	case shk::operand::type::reg:
		return operand.value;
	default:
		std::cerr << "error: eval_ref: invalid operand type" << std::endl;
		return 0;
	}
}

uint16_t eval(cpu_state &state, shk::operand &operand) {
	if(operand.ty == shk::operand::type::imm) {
		return operand.value;
	} else {
		return state.reg[eval_ref(state, operand)];
	}
}

bool execute(cpu_state &state) {
	if(auto instr = decode(state)) {
		for(auto &command : instr->commands) {
			switch(command.ty) {
			case shk::command::type::eq:
				if(!(eval(state, command.operands[0]) == 0u)) {
					return true;
				}
				break;
			case shk::command::type::lt: {
				auto x = eval(state, command.operands[0]);
				if(!(*reinterpret_cast<int16_t *>(&x) < 0)) {
					return true;
				}
				break;
			}
			case shk::command::type::le: {
				auto x = eval(state, command.operands[0]);
				if(!(*reinterpret_cast<int16_t *>(&x) <= 0)) {
					return true;
				}
				break;
			}
			default:
				std::cerr << "error: " << command.ty << " not implemented" << std::endl;
				return false;
			}
		}

		switch(instr->op) {
		case shk::opcode::noop:
			break;
		case shk::opcode::debug:
			debug(state);
			std::cout << "Hit enter to continue" << std::endl;
			getc(stdin);
			break;
		case shk::opcode::halt:
			std::cout << "Hit enter to continue" << std::endl;
			getc(stdin);
			break;
		case shk::opcode::die:
			return false;
		case shk::opcode::load:
			state.reg[eval_ref(state, instr->operands[0])] = state.mem[eval(state, instr->operands[1])];
			break;
		case shk::opcode::store:
			state.mem[eval(state, instr->operands[0])] = eval(state, instr->operands[1]);
			break;
		case shk::opcode::move:
			state.reg[eval_ref(state, instr->operands[0])] = eval(state, instr->operands[1]);
			break;
		case shk::opcode::add:
			state.reg[eval_ref(state, instr->operands[0])] = eval(state, instr->operands[1]) + eval(state, instr->operands[2]);
			break;
		case shk::opcode::compare:
			state.reg[eval_ref(state, instr->operands[0])] = eval(state, instr->operands[1]) - eval(state, instr->operands[2]);
			break;
		case shk::opcode::multiply:
			state.reg[eval_ref(state, instr->operands[0])] = eval(state, instr->operands[1]) * eval(state, instr->operands[2]);
			break;
		case shk::opcode::branch:
			state.reg[state.ip] = eval(state, instr->operands[0]);
			break;
		case shk::opcode::get_ip:
			state.reg[eval_ref(state, instr->operands[0])] = state.ip;
			break;
		case shk::opcode::set_ip:
			state.ip = eval_ref(state, instr->operands[0]);
			break;
		default:
			std::cerr << "error: " << instr->op << " not implemented" << std::endl;
			break;
		}
		std::cout << "executed " << instr->op << std::endl;
		return true;
	} else {
		return false;
	}
}

int main(int argc, char *argv[]) {
	cpu_state state;

	int opt;
	while((opt = getopt(argc, argv, "")) != -1) {
		switch(opt) {
		case '?':
			return 1;
		}
	}

	size_t in_count = argc - optind;
	if(in_count == 0) {
		std::cerr << argv[0] << ": no input files" << std::endl;
		return 1;
	}

	for(size_t i = 0; i < in_count; ++i) {
		char *in_path = argv[i + optind];

		std::ifstream is(in_path, std::ios::in | std::ios::binary);

		while(is.peek() != EOF) {
			uint8_t hi, lo;
			is.read(reinterpret_cast<char *>(&hi), 1);
			is.read(reinterpret_cast<char *>(&lo), 1);
			uint16_t byte = (uint16_t(hi) << 8u) | uint16_t(lo);
			state.mem[state.reg[state.ip]++] = byte;
		}
	}

	state.reg[state.ip] = 0;

	while(execute(state)) {}

	return 0;
}
