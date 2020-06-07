#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <vector>

#include <shk.h>

struct cpu_state {
	uint16_t mem[0x10000] = {0};
	uint16_t reg[0x100] = {0};
	uint16_t ip = 0;
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
	std::cout << "ip = " << state.ip << std::endl;
}

std::optional<shk::instruction> decode(cpu_state &state) {
	shk::instruction instr;

	if(state.mem[state.ip] >> 15u) {
		/*
		shk::command command;
		command.ty = static_cast<shk::command::type>(state.mem[state.ip] & 0xF);

		is.read(reinterpret_cast<char *>(&hi), 1);
		is.read(reinterpret_cast<char *>(&lo), 1);

		instr = decode_one(is);
		instr.commands.emplace_back(command);
		*/
		std::cerr << "commands not supported yet" << std::endl;
		return {};
	}

	instr.op = static_cast<shk::opcode>(state.mem[state.ip++]);
	for(size_t i = 0; i < shk::num_operands(instr.op); ++i) {
		auto byte = state.mem[state.ip++];

		shk::operand operand;
		operand.ty = static_cast<shk::operand::type>(byte >> 15u);
		operand.value = byte & 0xFF;
		instr.operands.emplace_back(operand);
	}

	std::cout << "decoded " << instr.op << std::endl;
	return instr;
}

uint16_t eval(cpu_state &state, shk::operand &operand) {
	switch(operand.ty) {
	case shk::operand::type::imm:
		return operand.value;
	case shk::operand::type::reg:
		return state.reg[operand.value];
	default:
		std::cerr << "error: invalid operand type" << std::endl;
		break;
	}

	return 0;
}

bool execute(cpu_state &state) {
	if(auto instr = decode(state)) {
		switch(instr->op) {
		case shk::opcode::noop:
			break;
		case shk::opcode::debug:
			debug(state);
			std::cout << "Hit enter to continue" << std::endl;
			getc(stdin);
			break;
		case shk::opcode::store:
			state.mem[instr->operands[0].value] = instr->operands[1].value;
			break;
		case shk::opcode::move:
			state.reg[instr->operands[0].value] = eval(state, instr->operands[1]);
			break;
		case shk::opcode::add:
			state.reg[instr->operands[0].value] = eval(state, instr->operands[1]) + eval(state, instr->operands[2]);
			break;
		case shk::opcode::branch:
			state.ip = instr->operands[0].value;
			break;
		default:
			std::cerr << "not implemented" << std::endl;
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
			state.mem[state.ip++] = byte;
		}
	}

	state.ip = 0;

	while(execute(state)) {}

	return 0;
}
