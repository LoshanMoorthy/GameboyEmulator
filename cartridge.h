#pragma once

#include <string>
#include <vector>

#include "cartridge_info.h"
#include "definitions.h"

class Cartridge {
public:
	Cartridge(std::string filename);

	u8 read(const class Address& address) const;

	std::string game_title() const;

private:
	std::vector<u8> data;

	// Metadata
	CartridgeType type;
	ROMSize rom_size;
	RAMSize ram_size;
	std::string license_code;
	u8 version;

	bool supports_cgb = false;
	bool supports_sgb = false;

	// Header has 'destination code' (Jap or Non-Jap)
	// TODO: Implement for next iteration
};