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

		uint16_t eval_ref(shk::operand &operand) {
			switch(operand.ty) {
			case shk::operand::type::imm:
				std::cerr << "error: eval_ref: cannot reference an immediate" << std::endl;
				return 0;
			case shk::operand::type::reg:
				return operand.value;
			case shk::operand::type::deref:
				return reg[operand.value];
			default:
				std::cerr << "error: eval_ref: invalid operand type" << std::endl;
				return 0;
			}
		}

		uint16_t eval(shk::operand &operand) {
			if(operand.ty == shk::operand::type::imm) {
				return operand.value;
			} else {
				return reg[eval_ref(operand)];
			}
		}

		std::optional<shk::instruction> decode() {
			shk::instruction instr;

			auto byte = mem[reg[ip]++];

			if(byte >> 15u) {
				shk::command command;
				command.ty = static_cast<shk::command::type>(byte & 0xFF);

				for(size_t i = 0; i < shk::num_operands(command.ty); ++i) {
					auto byte = mem[reg[ip]++];

					shk::operand operand;
					operand.ty = static_cast<shk::operand::type>(byte >> 14u);
					operand.value = byte & 0xFF;
					command.operands.emplace_back(operand);
				}

				auto ins = decode();
				if(!ins) {
					return {};
				}

				instr = *ins;
				instr.commands.emplace_back(command);
			} else {
				instr.op = static_cast<shk::opcode>(byte);
				for(size_t i = 0; i < shk::num_operands(instr.op); ++i) {
					auto byte = mem[reg[ip]++];

					shk::operand operand;
					operand.ty = static_cast<shk::operand::type>(byte >> 14u);
					operand.value = byte & 0xFF;
					instr.operands.emplace_back(operand);
				}
			}

			if(verbose) {
				std::cout << "decoded " << instr.op << std::endl;
			}
			return instr;
		}

		bool exec() {
			if(auto instr = decode()) {
				for(auto &command : instr->commands) {
					switch(command.ty) {
					case shk::command::type::eq:
						if(!(eval(command.operands[0]) == 0u)) {
							return true;
						}
						break;
					case shk::command::type::lt: {
						auto x = eval(command.operands[0]);
						if(!(*reinterpret_cast<int16_t *>(&x) < 0)) {
							return true;
						}
						break;
					}
					case shk::command::type::le: {
						auto x = eval(command.operands[0]);
						if(!(*reinterpret_cast<int16_t *>(&x) <= 0)) {
							return true;
						}
						break;
					}
					case shk::command::type::gt: {
						auto x = eval(command.operands[0]);
						if(!(*reinterpret_cast<int16_t *>(&x) > 0)) {
							return true;
						}
						break;
					}
					case shk::command::type::ge: {
						auto x = eval(command.operands[0]);
						if(!(*reinterpret_cast<int16_t *>(&x) >= 0)) {
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
					debug();
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
					reg[eval_ref(instr->operands[0])] = mem[eval(instr->operands[1])];
					break;
				case shk::opcode::store:
					mem[eval(instr->operands[0])] = eval(instr->operands[1]);
					break;
				case shk::opcode::move:
					reg[eval_ref(instr->operands[0])] = eval(instr->operands[1]);
					break;
				case shk::opcode::add:
					reg[eval_ref(instr->operands[0])] = eval(instr->operands[1]) + eval(instr->operands[2]);
					break;
				case shk::opcode::compare:
					reg[eval_ref(instr->operands[0])] = eval(instr->operands[1]) - eval(instr->operands[2]);
					break;
				case shk::opcode::multiply:
					reg[eval_ref(instr->operands[0])] = eval(instr->operands[1]) * eval(instr->operands[2]);
					break;
				case shk::opcode::branch:
					reg[ip] = eval(instr->operands[0]);
					break;
				case shk::opcode::call:
					--reg[sp];
					mem[reg[sp]] = reg[ip];
					reg[ip] = eval(instr->operands[0]);
					break;
				case shk::opcode::ret:
					reg[ip] = mem[reg[sp]];
					++reg[sp];
					break;
				case shk::opcode::get_ip:
					reg[eval_ref(instr->operands[0])] = ip;
					break;
				case shk::opcode::set_ip:
					ip = eval_ref(instr->operands[0]);
					break;
				case shk::opcode::get_sp:
					reg[eval_ref(instr->operands[0])] = sp;
					break;
				case shk::opcode::set_sp:
					sp = eval_ref(instr->operands[0]);
					break;
				default:
					std::cerr << "error: " << instr->op << " not implemented" << std::endl;
					break;
				}
				if(verbose) {
					std::cout << "executed " << instr->op << std::endl;
				}
				return true;
			} else {
				return false;
			}
		}

		void debug() {
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
			std::cout << std::endl;
		}

		void run() {
			reg[ip] = 0;
			reg[sp] = 0;

			while(exec()) {}
		}
	};
} // namespace shk
