#pragma once

#include <string>

#include "definitions.h"

const int TITLE_LENGTH = 11;

namespace header {
	const int entry_point		= 0x100;
	const int logo				= 0x104;
	const int title				= 0x134;
	const int manufacturer_code = 0x13F;
	const int cartridge_type	= 0x147;
	const int rom_size			= 0x148;
	const int ram_size			= 0x149;
	const int version_number	= 0x14C;
}

// Type
enum class CartridgeType {
	ROMOnly,
	MBC1,
	MBC2,
	MBC3,
	MBC4,
	MBC5,
	Unknown,
};

// Helper for reading the type code
CartridgeType get_type(u8 type);
std::string describe(CartridgeType type);

// ROM sizes
enum class ROMSize {
	KB32,
	KB64,
	KB128,
	KB256,
	KB512,
	MB1,
	MB2,
	MB4,
	MB1p1,
	MB1p2,
	MB1p5,
};
ROMSize get_rom_size(u8 size_code);
std::string describe(ROMSize size);

// RAM sizes
enum RAMSize {
	None,
	KB2,
	KB8,
	KB32,
	KB128,
	KB64,
};
RAMSize get_ram_size(u8 size_code);
std::string describe(RAMSize size);