#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <vector>

#include <shk.h>

struct cpu_state {
	uint16_t mem[0x10000];
	uint16_t reg[0x100];
	uint16_t ip;
};

void debug(cpu_state &state) {
	for(size_t i = 0x0; i < 0x8; ++i) {
		std::cout << "mem[";
		std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << i;
		std::cout << "] = ";
		std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << state.mem[i];
		std::cout << std::endl;
	}
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
		shk::operand operand;
		operand.ty = shk::operand::type::reg; // XXX
		operand.value = state.mem[state.ip++] & 0xFF;
		instr.operands.emplace_back(operand);
	}

	std::cout << "decoded " << instr.op << std::endl;
	return instr;
}

bool execute(cpu_state &state) {
	if(auto instr = decode(state)) {
		switch(instr->op) {
		case shk::opcode::debug:
			debug(state);
			std::cout << "Hit enter to continue" << std::endl;
			getc(stdin);
			break;
		case shk::opcode::store:
			state.mem[instr->operands[0].value] = instr->operands[1].value;
			break;
		default:
			break;
		}
		std::cout << "executed " << instr->op << std::endl;
		return true;
	} else {
		return false;
	}
}

int main(int argc, char *argv[]) {
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

	cpu_state state;
	state.ip = 0;

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

		/*
		auto instrs = decode(is);

		for(auto &instr : instrs) {
			std::cout << instr.op << std::endl;
			state.mem[state.ip++] = instr.op
		}
		*/
	}

	state.ip = 0;

	while(execute(state)) {}

	return 0;
}
