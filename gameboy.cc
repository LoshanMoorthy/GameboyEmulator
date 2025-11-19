#include "gameboy.h"
#include "cpu.h"
#include "mmu.h"
#include "video.h"
#include "cartridge.h"
#include "files.h"

Gameboy::Gameboy(const std::vector<u8>& cartridge_data,
                 Options& options,
                 const std::vector<u8>& save_data)
    : cartridge(get_cartridge(cartridge_data, save_data))
    , cpu(*this, nullptr, options)       
    , video(*this)                       
    , mmu(*cartridge, cpu, video, *this) 
{
    cpu.setMMUPointer(&mmu);

    if (options.disable_logs)
        log_set_level(LogLevel::Error);
    else if (options.trace)
        log_set_level(LogLevel::Trace);
    else
        log_set_level(LogLevel::Info);
}

void Gameboy::run(
    const should_close_callback_t& _should_close_callback,
    const vblank_callback_t& _vblank_callback
) {
    should_close_callback = _should_close_callback;
    video.register_vblank_callback(_vblank_callback);

    while (!should_close_callback()) {
        tick();
    }
}

void Gameboy::tick() {
    auto cycles = cpu.tick();
    elapsed_cycles += cycles.cycles;

    video.tick(cycles);
}

auto Gameboy::get_cartridge_ram() const -> const std::vector<u8>& {
    return cartridge->get_cartridge_ram();
}