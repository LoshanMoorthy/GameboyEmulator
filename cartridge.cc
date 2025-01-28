#include "cartridge.h"
#include "cartridge_info.h"
#include "definitions.h"
#include "files.h"
#include "log.h"

class Address {
public:
	Address(u16 address): addr(address) {}
	u16 value() const { return addr; }
private:
	u16 addr;
};

Cartridge::Cartridge(std::string filename) {
	auto rom_data = read_bytes(filename);
	log_info("Loaded %d KB FROM '%s'", (int)(rom_data.size() / 1024), filename.c_str());

	// Copy into our data vec
	data = std::vector<u8>(rom_data.begin(), rom_data.end());

	// Retrieve codes from standard Cartridhe header area:
	if (data.size() < 0x150) {
		fatal_error("Cartridge file too small, or not a valid GB ROM?");
	}

	u8 type_code	 = data[header::cartridge_type];
	u8 version_code  = data[header::version_number];
	u8 rom_size_code = data[header::rom_size];
	u8 ram_size_code = data[header::ram_size];

	// Convert to enumerations:
	type	 = get_type(type_code);
	version  = version_code;
	rom_size = get_rom_size(rom_size_code);
	ram_size = get_ram_size(ram_size_code);

	log_info("Title:      '%s' (version %d)", game_title().c_str(), version);
	log_info("Cartridge:  %s", describe(type).c_str());
	log_info("ROM Size:   %s", describe(rom_size).c_str());
	log_info("RAM Size:   %s", describe(ram_size).c_str());
}

u8 Cartridge::read(const Address& address) const {
	if (address.value() < data.size()) {
		return data[address.value()];
	}
	return 0xFF;
}

std::string Cartridge::game_title() const {
	// Title is at offset 0x134? 
	char name[TITLE_LENGTH + 1] = {0};
	for (int i = 0; i < TITLE_LENGTH; i++) {
		name[i] = (char)data[header::title + i];
	}

	// Trim trailing spaces
	std::string raw(name);
	while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\0'))
		raw.pop_back();
	return raw;
}