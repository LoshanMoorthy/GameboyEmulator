#include <iostream>
#include <vector>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "gameboy.h"
#include "cli.h"
#include "files.h"
#include "log.h"
#include "framebuffer.h"

static const int GB_WIDTH = 160;
static const int GB_HEIGHT = 144;
static const int SCALE = 3; // 480x432 window

// GB Color enum -> ARGB pixel
static uint32_t color_to_pixel(Color c) {
    switch (c) {
    case Color::White:     return 0xFFFFFFFF;
    case Color::LightGray: return 0xFFAAAAAA;
    case Color::DarkGray:  return 0xFF555555;
    case Color::Black:     return 0xFF000000;
    default:               return 0xFFFFFFFF;
    }
}

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* texture = nullptr;
static bool          quit = false;

static bool should_close_callback() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = true;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;
    }
    return quit;
}

static void vblank_callback(const FrameBuffer& fb) {
    static int frame_count = 0;
    frame_count++;
    if (frame_count == 1) {
        fprintf(stderr, "First frame! Check window.\n");
    }
    static uint32_t pixels[GB_WIDTH * GB_HEIGHT];
    for (int y = 0; y < GB_HEIGHT; y++) {
        for (int x = 0; x < GB_WIDTH; x++) {
            pixels[y * GB_WIDTH + x] = color_to_pixel(fb.get_pixel(x, y));
        }
    }

    SDL_UpdateTexture(texture, nullptr, pixels, GB_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    Options options = get_options(argc, argv);
    options.disable_logs = true;

    auto rom_char = read_bytes(options.filename);
    if (rom_char.empty()) {
        std::cerr << "Failed to read ROM: " << options.filename << "\n";
        return 1;
    }

    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    window = SDL_CreateWindow(
        "Game Boy Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GB_WIDTH * SCALE, GB_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, GB_WIDTH * SCALE, GB_HEIGHT * SCALE);

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        GB_WIDTH, GB_HEIGHT
    );
    if (!texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        return 1;
    }

    std::vector<u8> rom_data(rom_char.begin(), rom_char.end());
    std::vector<u8> save_data;
    Gameboy gb(rom_data, options, save_data);

    gb.run(should_close_callback, vblank_callback);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}