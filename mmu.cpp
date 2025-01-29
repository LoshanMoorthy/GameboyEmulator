#include "mmu.h"
#include "log.h"
#include "cpu.h"
#include "video.h"
#include "boot.h"

MMU::MMU(Cartridge& inCartridge, CPU& inCPU, Video& inVideo)
    : cartridge(inCartridge)
    , cpu(inCPU)
    , video(inVideo) {
    // Allocate the full 64KB address space with 0s.
    memory.resize(0x10000, 0);
}

u8 MMU::read(const Address& address) const {
    u16 addr = address.value();

    // 0x0000..0x7FFF
    if (addr < 0x8000) {
        // TODO: boot_rom_active
        return cartridge.read(address);
    }
    // 0x8000..0x9FFF is VRAM, for now memory[]:
    if (addr < 0xA000) {
        return memory_read(address);
    }
    // 0xA000..0xBFFF is "External RAM" in the Cartridge:
    if (addr < 0xC000) {
        // Some MBC controllers put RAM here, but we can just:
        return memory_read(address);
    }
    // 0xC000..0xDFFF is work RAM:
    if (addr < 0xE000) {
        return memory_read(address);
    }
    // 0xE000..0xFDFF is the "echo" of C000..DDFF. For now:
    if (addr < 0xFE00) {
        // mirrored region: read from (addr - 0x2000)
        return memory_read(Address(addr - 0x2000));
    }
    // 0xFE00..0xFE9F = OAM
    if (addr < 0xFEA0) {
        return memory_read(address);
    }
    // 0xFEA0..0xFEFF is "unusable" (generally returns 0xFF or open bus).
    if (addr < 0xFF00) {
        // We'll return 0xFF if needed:
        return 0xFF;
    }
    // 0xFF00..0xFF7F = I/O Registers
    if (addr < 0xFF80) {
        return read_io(address);
    }
    // 0xFF80..0xFFFE = High (Zero-page) RAM
    if (addr < 0xFFFF) {
        return memory_read(address);
    }
    // 0xFFFF = Interrupt Enable register
    // For now, we just read from memory vector:
    return memory_read(address);
}

void MMU::write(const Address& address, u8 byte) {
    u16 addr = address.value();

    // 0x0000..0x7FFF is ROM => writes often go to MBC for bank-switching, etc.
    if (addr < 0x8000) {
        log_warn("Attempted write to ROM area 0x%04X = 0x%02X", addr, byte);
        // We'll no-op or pass to MBC if implemented.
        return;
    }
    else if (addr < 0xA000) {
        // VRAM
        memory_write(address, byte);
        return;
    }
    else if (addr < 0xC000) {
        // Cartridge (External) RAM region
        memory_write(address, byte);
        return;
    }
    else if (addr < 0xE000) {
        // Work RAM
        memory_write(address, byte);
        return;
    }
    else if (addr < 0xFE00) {
        // Echo RAM
        memory_write(Address(addr - 0x2000), byte);
        return;
    }
    else if (addr < 0xFEA0) {
        // OAM
        memory_write(address, byte);
        return;
    }
    else if (addr < 0xFF00) {
        // Unusable
        log_warn("Write to unusable area 0x%04X = 0x%02X", addr, byte);
        return;
    }
    else if (addr < 0xFF80) {
        // I/O
        write_io(address, byte);
        return;
    }
    else if (addr < 0xFFFF) {
        // High RAM
        memory_write(address, byte);
        return;
    }
    else {
        // 0xFFFF = IE register
        memory_write(address, byte);
        return;
    }
}

bool MMU::boot_rom_active() const {
    return false;
}

u8 MMU::read_io(const Address& address) const {
    u16 addr = address.value();
    log_debug("MMU read from I/O register 0x%04X (stub)", addr);
    return memory_read(address);
}

void MMU::write_io(const Address& address, u8 byte) {
    u16 addr = address.value();
    log_debug("MMU write to I/O register 0x%04X = 0x%02X (stub)", addr, byte);
    memory_write(address, byte);
}

u8 MMU::memory_read(const Address& address) const {
    return memory[address.value()];
}

void MMU::memory_write(const Address& address, u8 byte) {
    memory[address.value()] = byte;
}