#include <iostream>

#include "cpu.h"
#include "video.h"
#include "mmu.h"
#include "cartridge.h"
#include "log.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " path/to/rom.gb\n";
        return 1;
    }

    Options opts;
    opts.trace = true;

    log_set_level(LogLevel::Trace);

    Cartridge cart(argv[1]);

    CPU cpu(nullptr, opts);
    Video dummyVideo; 
    MMU mmu(cart, cpu, dummyVideo);
    cpu.setMMUPointer(&mmu);

    for (int i = 0; i < 200; i++) {
        Cycles c = cpu.tick();
        std::cout << "CPU tick => " << c.cycles << " cycles\n";
    }

    return 0;
}