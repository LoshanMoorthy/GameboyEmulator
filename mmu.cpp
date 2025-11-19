#include "mmu.h"
#include "log.h"
#include "cpu.h"
#include "video.h"
#include "boot.h"
#include "gameboy.h"

MMU::MMU(Cartridge& inCartridge, CPU& inCPU, Video& inVideo, Gameboy& inGb)
    : cartridge(inCartridge)
    , cpu(inCPU)
    , video(inVideo)
    , gameboy(inGb) {
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
        return memory_read(address);
    }
    // 0xC000..0xDFFF is work RAM:
    if (addr < 0xE000) {
        return memory_read(address);
    }
    // 0xE000..0xFDFF is the "echo" of C000..DDFF. For now:
    if (addr < 0xFE00) {
        return memory_read(Address(addr - 0x2000));
    }
    // 0xFE00..0xFE9F = OAM
    if (addr < 0xFEA0) {
        return memory_read(address);
    }
    if (addr < 0xFF00) {
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
    return memory_read(address);
}

void MMU::write(const Address& address, u8 byte) {
    u16 addr = address.value();

    // 0x0000..0x7FFF is ROM => writes often go to MBC for bank-switching, etc.
    if (addr < 0x8000) {
        log_warn("Attempted write to ROM area 0x%04X = 0x%02X", addr, byte);
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

    if (addr >= 0xFF40 && addr <= 0xFF4B) {
        switch (addr) {
        case 0xFF40: return video.lcd_control.value();
        case 0xFF41: return video.lcd_status.value();
        case 0xFF42: return video.scroll_y.value();
        case 0xFF43: return video.scroll_x.value();
        case 0xFF44: return video.line.value();
        case 0xFF45: return video.ly_compare.value();
        case 0xFF46: return video.dma_transfer.value();
        case 0xFF47: return video.bg_palette.value();
        case 0xFF48: return video.sprite_palette_0.value();
        case 0xFF49: return video.sprite_palette_1.value();
        case 0xFF4A: return video.window_y.value();
        case 0xFF4B: return video.window_x.value();
        }
    }

    return memory_read(address);
}

void MMU::write_io(const Address& address, u8 byte) {
    u16 addr = address.value();
    if (addr >= 0xFF40 && addr <= 0xFF4B) {
        switch (addr) {
        case 0xFF40: video.lcd_control.set(byte); break;
        case 0xFF41: video.lcd_status.set(byte); break;
        case 0xFF42: video.scroll_y.set(byte); break;
        case 0xFF43: video.scroll_x.set(byte); break;
        case 0xFF44: video.line.set(0); break; 
        case 0xFF45: video.ly_compare.set(byte); break;
        case 0xFF46: video.dma_transfer.set(byte); break;
        case 0xFF47: video.bg_palette.set(byte); break;
        case 0xFF48: video.sprite_palette_0.set(byte); break;
        case 0xFF49: video.sprite_palette_1.set(byte); break;
        case 0xFF4A: video.window_y.set(byte); break;
        case 0xFF4B: video.window_x.set(byte); break;
        }
        return;
    }

    memory_write(address, byte);
}

u8 MMU::memory_read(const Address& address) const {
    return memory[address.value()];
}

void MMU::memory_write(const Address& address, u8 byte) {
    memory[address.value()] = byte;
}