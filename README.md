# Game Boy Emulator (WIP)

A work-in-progress **Nintendo Game Boy emulator** written in modern C++.

I started this project to learn how the real hardware works - CPU, memory map and the PPU.

> ⚠️ Status: Experimental / incomplete.  
> Work in progress – not a playable emulator yet.

---

## 🎯 Goals

- Emulate the original Game Boy hardware at a low level
- Implement the LR35902 CPU (registers, flags, opcodes, timing)
- Model the full 64KB memory map through an MMU
- Implement scanline-based video rendering (background, window, sprites)
- Learn how real hardware coordinates CPU, PPU, interrupts and timing

## What it does right now

- Loads a ROM from disk
- Implements the LR35902 CPU with:
  - 8-bit/16-bit registers
  - Flags (Z, N, H, C)
  - A large part of the instruction set (ADD/ADC/SUB/SBC, AND/OR/XOR, BIT/SET/RES, shifts/rotates, JP/JR/CALL/RET, etc.)
- Has an MMU that models the 64KB address space:
  - Cartridge ROM, work RAM, echo RAM, OAM, high RAM
  - Basic handling of LCD I/O registers (0xFF40–0xFF4B)
- A simple PPU/Video system:
  - Tracks LCD modes (OAM, VRAM, HBlank, VBlank)
  - Increments LY per scanline
  - Renders the background line-by-line using tile maps and tile data into a `FrameBuffer`
  - Calls a VBlank callback once a frame is ready
 
## Building & running

The project is set up as a Visual Studio C++ project.

1. Clone the repo and open the solution in Visual Studio.
2. Build it (Debug or Release).
3. Run it with a ROM file:

```bash
GameboyEmulator.exe path\to\rom.gb
