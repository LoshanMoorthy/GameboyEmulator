#include <iostream>
#include <vector>
#include "gameboy.h"
#include "cli.h"       
#include "files.h"      
#include "log.h"
#include "framebuffer.h"

static bool my_should_close_callback() {
    static int counter = 0;
    return (++counter > 60);
}

static void my_vblank_callback(const FrameBuffer& fb) {
    static int frameCount = 0;
    frameCount++;
    log_info("VBlank callback! frame = %d", frameCount);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <romfile.gb>\n";
        return 1;
    }
    std::string rom_path = argv[1];

    Options options;
    options.trace = true;
    options.disable_logs = false;

    auto rom_char = read_bytes(rom_path);
    if (rom_char.empty()) {
        std::cerr << "Failed to read ROM from " << rom_path << "\n";
        return 1;
    }

    std::vector<u8> rom_data(rom_char.begin(), rom_char.end());

    std::vector<u8> save_data;

    Gameboy gb(rom_data, options, save_data);

    gb.run(
        my_should_close_callback,
        my_vblank_callback
    );

    std::cout << "Exiting emulator\n";
    return 0;
}
