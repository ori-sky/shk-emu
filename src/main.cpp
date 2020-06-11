#include <unistd.h>

#include <fstream>

#include <shk/emu.h>

int main(int argc, char *argv[]) {
	bool verbose = false;

	int opt;
	while((opt = getopt(argc, argv, "v")) != -1) {
		switch(opt) {
		case 'v':
			verbose = true;
			break;
		case '?':
			return 1;
		}
	}

	size_t in_count = argc - optind;
	if(in_count == 0) {
		std::cerr << argv[0] << ": no input files" << std::endl;
		return 1;
	}

	shk::emulator emulator(verbose);

	for(size_t i = 0; i < in_count; ++i) {
		char *in_path = argv[i + optind];

		std::ifstream is(in_path, std::ios::in | std::ios::binary);
		if(is.fail()) {
			std::cerr << argv[0] << ": " << in_path << ": failed to open" << std::endl;
			return 1;
		}

		if(!emulator.load(is)) {
			return 2;
		}
	}

	emulator.run();

	return 0;
}
