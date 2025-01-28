#include <iostream>

#include "cartridge.h"
#include "log.h"

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " path/to/rom.gb\n";
		return 1;
	}

	log_set_level(LogLevel::Info);

	try {
		Cartridge cart(argv[1]);
		std::cout << "Cartridge loaded sucessfully!\n";
	}
	catch (...) {
		std::cerr << "Failed to load Cartridge.\n";
		return 1;
	}

	return 0;
}