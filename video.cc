#include "video.h"
#include "color.h"
#include "tile.h"
#include "cpu.h"
#include "cartridge.h"
#include "bitwise.h"
#include "log.h"
#include "gameboy.h"

using bitwise::check_bit;

Video::Video(Gameboy& inGb)
    : gb(inGb)
    , buffer(GAMEBOY_WIDTH, GAMEBOY_HEIGHT) {
    // Initialize registers to 0 if needed
    lcd_control.set(0);
    lcd_status.set(0);
    scroll_y.set(0);
    scroll_x.set(0);
    line.set(0);
    ly_compare.set(0);
    dma_transfer.set(0);
    bg_palette.set(0xFC);         
    sprite_palette_0.set(0xFF);
    sprite_palette_1.set(0xFF);
    window_y.set(0);
    window_x.set(0);
}

// Called after each CPU instruction. 'cycles' = how many cycles that instruction used.
void Video::tick(Cycles cycles) {
    cycle_counter += cycles.cycles;

    switch (current_mode) {
    case VideoMode::ACCESS_OAM:
    if (cycle_counter >= CLOCKS_PER_SCANLINE_OAM) {
        // Switch to ACCESS_VRAM
        cycle_counter -= CLOCKS_PER_SCANLINE_OAM;
        current_mode = VideoMode::ACCESS_VRAM;
        // LCD STAT bits etc.
    }
    break;

    case VideoMode::ACCESS_VRAM:
    if (cycle_counter >= CLOCKS_PER_SCANLINE_VRAM) {
        // Switch to HBLANK
        cycle_counter -= CLOCKS_PER_SCANLINE_VRAM;
        current_mode = VideoMode::HBLANK;
    }
    break;

    case VideoMode::HBLANK: {
        if (cycle_counter >= CLOCKS_PER_HBLANK) {
            cycle_counter -= CLOCKS_PER_HBLANK;

            write_scanline(line.value());

            line.increment();

            // If LY == 144 => enter VBLANK
            if (line.value() == 144) {
                current_mode = VideoMode::VBLANK;

                // set VBlank interrupt bit (0)
                gb.cpu.interrupt_flag.set_bit_to(0, true);
            }
            else {
                // Go back to OAM
                current_mode = VideoMode::ACCESS_OAM;
            }
        }
        break;
    }
    case VideoMode::VBLANK:
    // We stay in VBLANK for 10 lines (144..153) => total 456 * 10 cycles
    if (line.value() >= 144 && line.value() < 154) {
        // each line is 456 cycles
        if (cycle_counter >= CLOCKS_PER_SCANLINE) {
            cycle_counter -= CLOCKS_PER_SCANLINE;
            line.increment();

            // If we pass line 153 => wrap to 0
            if (line.value() > 153) {
                line.set(0);
                current_mode = VideoMode::ACCESS_OAM;

                // End of VBlank => we can render the entire frame
                write_sprites();
                draw(); // call the final vblank callback
                buffer.reset();
            }
        }
    }
    break;
    }
}

bool Video::display_enabled() const { return check_bit(lcd_control.value(), 7); }
bool Video::window_tile_map() const { return check_bit(lcd_control.value(), 6); }
bool Video::window_enabled() const { return check_bit(lcd_control.value(), 5); }
bool Video::bg_window_tile_data() const { return check_bit(lcd_control.value(), 4); }
bool Video::bg_tile_map_display() const { return check_bit(lcd_control.value(), 3); }
bool Video::sprite_size() const { return check_bit(lcd_control.value(), 2); }
bool Video::sprites_enabled() const { return check_bit(lcd_control.value(), 1); }
bool Video::bg_enabled() const { return check_bit(lcd_control.value(), 0); }

void Video::write_scanline(u8 current_line) {
    if (!display_enabled()) return;

    if (bg_enabled()) {
        draw_bg_line(current_line);
    }
    if (window_enabled()) {
        draw_window_line(current_line);
    }
}

// After all lines are drawn, we can draw all sprites
void Video::write_sprites() {
    if (!sprites_enabled()) return;

    for (uint spriteIndex = 0; spriteIndex < 40; spriteIndex++) {
        draw_sprite(spriteIndex);
    }
}

// Renders background for one line
void Video::draw_bg_line(uint current_line) {
    // Typical addresses
    const Address TILE_SET_ZERO_ADDRESS = 0x8000;
    const Address TILE_SET_ONE_ADDRESS = 0x8800;

    const Address TILE_MAP_ZERO_ADDRESS = 0x9800;
    const Address TILE_MAP_ONE_ADDRESS = 0x9C00;

    // Decide which tile set
    bool use_tile_set_zero = bg_window_tile_data(); // 0x8000
    Address tile_set = use_tile_set_zero ? TILE_SET_ZERO_ADDRESS : TILE_SET_ONE_ADDRESS;

    // Decide which BG tile map
    bool use_tile_map_zero = !bg_tile_map_display();
    Address tile_map = use_tile_map_zero ? TILE_MAP_ZERO_ADDRESS : TILE_MAP_ONE_ADDRESS;

    // For each pixel in [0..159]
    for (uint screen_x = 0; screen_x < GAMEBOY_WIDTH; screen_x++) {
        uint scrolled_x = screen_x + scroll_x.value();
        uint scrolled_y = current_line + scroll_y.value();

        // BG is 256×256, repeated tile map
        uint tile_map_x = scrolled_x % 256;
        uint tile_map_y = scrolled_y % 256;

        // Which tile in the tilemap?
        uint tile_x = tile_map_x / 8;
        uint tile_y = tile_map_y / 8;
        uint tile_index = tile_y * 32 + tile_x;

        // read tile ID
        u8 tile_id = gb.mmu.read(tile_map + tile_index);

        // If using tile_set_one, interpret tile_id as signed
        s16 tile_number = use_tile_set_zero ? tile_id : (s8)tile_id + 128;

        // compute tile address
        Address tile_address = tile_set + (tile_number * 16);

        // pixel inside the tile
        uint pixel_x = tile_map_x % 8;
        uint pixel_y = tile_map_y % 8;

        // read tile
        Tile tile(tile_address, gb.mmu);
        GBColor colorIdx = tile.get_pixel(pixel_x, pixel_y);

        // apply the BG palette
        auto palette = load_palette(bg_palette);
        auto final_color = get_color_from_palette(colorIdx, palette);

        buffer.set_pixel(screen_x, current_line, final_color);
    }
}

static Color get_real_color(u8 pixel_value) {
    // typical “DMG” shading
    switch (pixel_value) {
    case 0: return Color::White;
    case 1: return Color::LightGray;
    case 2: return Color::DarkGray;
    case 3: return Color::Black;
    default:
    fatal_error("Invalid color index for real color: %d", pixel_value);
    return Color::White;
    }
}

// Renders window (similar logic) for one line
void Video::draw_window_line(uint current_line) {
    if (current_line < window_y.value()) {
        return; // Window not yet on screen
    }
    uint win_line = current_line - window_y.value();
    if (win_line >= GAMEBOY_HEIGHT) return;
}

void Video::draw_sprite(uint sprite_n) {
    // OAM is at 0xFE00
    Address oam_start = Address(0xFE00 + sprite_n * 4);

    u8 posY = gb.mmu.read(oam_start + 0); // Y pos
    u8 posX = gb.mmu.read(oam_start + 1); // X pos
    u8 tileNum = gb.mmu.read(oam_start + 2);
    u8 attr = gb.mmu.read(oam_start + 3);
}

Palette Video::load_palette(const ByteRegister& palette_reg) {
    using bitwise::compose_bits;
    auto p = palette_reg.value();

    // Each pair of bits is a color
    // bits 0-1 => color0, bits 2-3 => color1, bits 4-5 => color2, bits 6-7 => color3
    u8 c0 = p & 0x03;         
    u8 c1 = (p >> 2) & 0x03;  
    u8 c2 = (p >> 4) & 0x03;
    u8 c3 = (p >> 6) & 0x03;

    auto col0 = get_real_color(c0);
    auto col1 = get_real_color(c1);
    auto col2 = get_real_color(c2);
    auto col3 = get_real_color(c3);

    return { col0, col1, col2, col3 };
}

Color Video::get_color_from_palette(GBColor color, const Palette& pal) {
    switch (color) {
    case GBColor::Color0: return pal.color0;
    case GBColor::Color1: return pal.color1;
    case GBColor::Color2: return pal.color2;
    case GBColor::Color3: return pal.color3;
    }
}

void Video::register_vblank_callback(const vblank_callback_t& cb) {
    vblank_callback = cb;
}

// Called at end of VBlank (or start?), to deliver the final buffer
void Video::draw() {
    if (vblank_callback) {
        vblank_callback(buffer);
    }
}