#include <unistd.h>

#include <fstream>
#include <vector>

#include <shk.h>

shk::instruction decode_one(std::istream &is) {
	shk::instruction instr;

	uint8_t hi, lo;
	is.read(reinterpret_cast<char *>(&hi), 1);
	is.read(reinterpret_cast<char *>(&lo), 1);
	uint16_t byte = (uint16_t(hi) << 8u) | uint16_t(lo);

	if(byte >> 15u) {
		shk::command command;
		command.ty = static_cast<shk::command::type>(byte & 0xF);

		is.read(reinterpret_cast<char *>(&hi), 1);
		is.read(reinterpret_cast<char *>(&lo), 1);

		instr = decode_one(is);
		instr.commands.emplace_back(command);
	} else {
		instr.op = static_cast<shk::opcode>(byte);

		for(size_t i = 0; i < shk::num_operands(instr.op); ++i) {
			is.read(reinterpret_cast<char *>(&hi), 1);
			is.read(reinterpret_cast<char *>(&lo), 1);
		}
	}

	return instr;
}

std::vector<shk::instruction> decode(std::istream &is) {
	std::vector<shk::instruction> instrs;

	while(is.peek() != EOF) {
		instrs.emplace_back(decode_one(is));
	}

	return instrs;
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

	for(size_t i = 0; i < in_count; ++i) {
		char *in_path = argv[i + optind];

		std::ifstream is(in_path, std::ios::in | std::ios::binary);

		auto instrs = decode(is);

		for(auto &instr : instrs) {
			std::cout << instr.op << std::endl;
		}
	}

	return 0;
}
