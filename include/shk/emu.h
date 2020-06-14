#pragma once

#include <iomanip>

#include <shk.h>

namespace shk {
	class emulator {
	  private:
		uint16_t mem[0x10000] = {0};
		uint16_t reg[0x100] = {0};
		uint16_t ip = 0xFF;
		uint16_t sp = 0xFE;
	  public:
		bool verbose;

		emulator(bool verbose = false) : verbose(verbose) {}

		bool load(std::istream &is) {
			while(is.peek() != EOF) {
				uint8_t hi, lo;
				is.read(reinterpret_cast<char *>(&hi), 1);
				is.read(reinterpret_cast<char *>(&lo), 1);
				uint16_t byte = (uint16_t(hi) << 8u) | uint16_t(lo);
				mem[reg[ip]++] = byte;
			}
			return true;
		}

		uint16_t eval_ref(operand &oper) const {
			switch(oper.ty) {
			case operand::type::imm:
				std::cerr << "error: eval_ref: cannot reference an immediate" << std::endl;
				return 0;
			case operand::type::reg:
				return oper.value;
			case operand::type::deref:
				return reg[oper.value];
			default:
				std::cerr << "error: eval_ref: invalid operand type" << std::endl;
				return 0;
			}
		}

		uint16_t eval(operand &oper) const {
			if(oper.ty == operand::type::imm) {
				return oper.value;
			} else {
				return reg[eval_ref(oper)];
			}
		}

		operand decode_operand() {
			auto byte = mem[reg[ip]++];

			operand oper;
			oper.ty = static_cast<operand::type>((byte >> 12u) & 0b11);
			oper.value = byte & 0xFFF;

			if(byte >> 15u) {
				auto oper2 = decode_operand();
				oper2.segment = std::make_unique<operand>(std::move(oper));
				return oper2;
			} else {
				return oper;
			}
		}

		std::optional<instruction> decode() {
			instruction instr;

			auto byte = mem[reg[ip]++];

			if(byte >> 15u) {
				command cmd;
				cmd.ty = static_cast<command::type>(byte & 0xFF);

				for(size_t i = 0; i < num_operands(cmd.ty); ++i) {
					cmd.operands.emplace_back(decode_operand());
				}

				auto ins = decode();
				if(!ins) {
					return {};
				}

				instr = std::move(*ins);
				instr.commands.emplace_back(std::move(cmd));
			} else {
				instr.op = static_cast<opcode>(byte);
				for(size_t i = 0; i < num_operands(instr.op); ++i) {
					instr.operands.emplace_back(decode_operand());
				}
			}

			if(verbose) {
				std::cout << "decoded " << instr.op << std::endl;
			}
			return instr;
		}

		bool exec(instruction &instr) {
			for(auto &cmd : instr.commands) {
				switch(cmd.ty) {
				case command::type::eq:
					if(!(eval(cmd.operands[0]) == 0u)) {
						return true;
					}
					break;
				case command::type::ne:
					if(!(eval(cmd.operands[0]) != 0u)) {
						return true;
					}
					break;
				case command::type::lt: {
					auto x = eval(cmd.operands[0]);
					if(!(*reinterpret_cast<int16_t *>(&x) < 0)) {
						return true;
					}
					break;
				}
				case command::type::le: {
					auto x = eval(cmd.operands[0]);
					if(!(*reinterpret_cast<int16_t *>(&x) <= 0)) {
						return true;
					}
					break;
				}
				case command::type::gt: {
					auto x = eval(cmd.operands[0]);
					if(!(*reinterpret_cast<int16_t *>(&x) > 0)) {
						return true;
					}
					break;
				}
				case command::type::ge: {
					auto x = eval(cmd.operands[0]);
					if(!(*reinterpret_cast<int16_t *>(&x) >= 0)) {
						return true;
					}
					break;
				}
				default:
					std::cerr << "error: " << cmd.ty << " not implemented" << std::endl;
					return false;
				}
			}

			switch(instr.op) {
			case opcode::noop:
				break;
			case opcode::debug:
				if(!debug()) {
					return false;
				}
				break;
			case opcode::halt:
				std::cout << "Hit enter to continue" << std::endl;
				getc(stdin);
				break;
			case opcode::die:
				return false;
			case opcode::load: {
				auto seg = instr.operands[1].segment ? eval(*instr.operands[1].segment) : 0;
				switch(seg) {
				case 0:
					reg[eval_ref(instr.operands[0])] = mem[eval(instr.operands[1])];
					break;
				case 1: {
					int c = getc(stdin);
					reg[eval_ref(instr.operands[0])] = c;
					break;
				}
				default:
					std::cerr << "error: unknown segment " << seg << std::endl;
					return false;
				}
				break;
			}
			case opcode::store: {
				auto seg = instr.operands[0].segment ? eval(*instr.operands[0].segment) : 0;
				switch(seg) {
				case 0:
					mem[eval(instr.operands[0])] = eval(instr.operands[1]);
					break;
				case 1:
					std::cout << char(eval(instr.operands[1]));
					break;
				default:
					std::cerr << "error: unknown segment " << seg << std::endl;
					return false;
				}
				break;
			}
			case opcode::pop:
				reg[eval_ref(instr.operands[0])] = mem[reg[sp]];
				++reg[sp];
				break;
			case opcode::push:
				--reg[sp];
				mem[reg[sp]] = eval(instr.operands[0]);
				break;
			case opcode::move:
				reg[eval_ref(instr.operands[0])] = eval(instr.operands[1]);
				break;
			case opcode::add:
				reg[eval_ref(instr.operands[0])] = eval(instr.operands[1]) + eval(instr.operands[2]);
				break;
			case opcode::compare:
				reg[eval_ref(instr.operands[0])] = eval(instr.operands[1]) - eval(instr.operands[2]);
				break;
			case opcode::multiply:
				reg[eval_ref(instr.operands[0])] = eval(instr.operands[1]) * eval(instr.operands[2]);
				break;
			case opcode::divide:
				reg[eval_ref(instr.operands[0])] = eval(instr.operands[1]) / eval(instr.operands[2]);
				break;
			case opcode::modulo:
				reg[eval_ref(instr.operands[0])] = eval(instr.operands[1]) % eval(instr.operands[2]);
				break;
			case opcode::branch:
				reg[ip] = eval(instr.operands[0]);
				break;
			case opcode::call:
				--reg[sp];
				mem[reg[sp]] = reg[ip];
				reg[ip] = eval(instr.operands[0]);
				break;
			case opcode::ret:
				reg[ip] = mem[reg[sp]];
				++reg[sp];
				break;
			case opcode::get_ip:
				reg[eval_ref(instr.operands[0])] = ip;
				break;
			case opcode::set_ip:
				ip = eval_ref(instr.operands[0]);
				break;
			case opcode::get_sp:
				reg[eval_ref(instr.operands[0])] = sp;
				break;
			case opcode::set_sp:
				sp = eval_ref(instr.operands[0]);
				break;
			default:
				std::cerr << "error: " << instr.op << " not implemented" << std::endl;
				break;
			}
			if(verbose) {
				std::cout << "executed " << instr.op << std::endl;
			}
			return true;
		}

		bool debug() {
			/*
			for(size_t i = 0x0; i < 0x8; ++i) {
				std::cout << "mem[";
				std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << i;
				std::cout << "] = ";
				std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << mem[i];
				std::cout << "   ";
				std::cout << "reg[";
				std::cout << "0x" << std::setfill('0') << std::setw(2) << std::hex << i;
				std::cout << "] = ";
				std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << reg[i];
				std::cout << std::endl;
			}
			std::cout << "ip = ";
			std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << reg[ip];
			std::cout << "   ";
			std::cout << "sp = ";
			std::cout << "0x" << std::setfill('0') << std::setw(4) << std::hex << reg[sp];
			std::cout << std::endl;
			*/

			std::string cmd;
			for(;;) {
				std::cout << "> ";
				std::string line;
				std::getline(std::cin, line);

				if(!line.empty()) {
					cmd = line;
				}

				if(cmd == "q") {
					break;
				} else if(cmd == "p") {
					for(size_t i = 0; i < 2; ++i) {
						std::cout << '$' << i << " = " << reg[i] << std::endl;
					}
				} else if(cmd == "si") {
					auto instr = decode();
					if(!instr) {
						return false;
					}

					if(auto mnemonic = opcode_to_mnemonic(instr->op)) {
						std::cout << *mnemonic;
						for(size_t i = 0; i < instr->operands.size(); ++i) {
							if(i > 0) {
								std::cout << ',';
							}
							std::cout << ' ' << instr->operands[i].type_char();
							std::cout << instr->operands[i].value;
						}
						for(auto &cmd : instr->commands) {
							if(auto mnemonic = command_to_mnemonic(cmd.ty)) {
								if(!instr->operands.empty()) {
									std::cout << ',';
								}
								std::cout << " !" << *mnemonic;
								for(size_t i = 0; i < cmd.operands.size(); ++i) {
									std::cout << ' ' << cmd.operands[i].type_char();
									std::cout << cmd.operands[i].value;
								}
							}
						}
						std::cout << std::endl;
					}

					exec(*instr);
				}
			}

			return true;
		}

		void run() {
			reg[ip] = 0;
			reg[sp] = 0;

			while(auto instr = decode()) {
				exec(*instr);
			}
		}
	};
} // namespace shk
