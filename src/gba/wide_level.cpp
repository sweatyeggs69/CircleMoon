// Widescreen support specific to Castlevania: Circle of the Moon (USA, AAME).
//
// The game keeps one 0x40-byte descriptor per background in EWRAM, in BG order, starting at
// 0x020230F0:
//
//   +0x02 u16 BGxCNT value      +0x04 u32 block table (ROM)    +0x0C u32 block map (EWRAM)
//   +0x10 u16 width, +0x12 u16 height (in blocks)              +0x14 u32 screen block (VRAM)
//   +0x18 s32 camera x, +0x1C s32 camera y (16.16 room pixels)
//
// Rooms are built from 32x32-pixel blocks. The block map holds one u16 block number per block,
// row by row (width entries per row), and each block is 16 tilemap entries in the table (4 rows
// of 4 tiles). The game streams the visible part of each room layer into a 512x512 VRAM map, so
// VRAM holds room pixel (x, y) at (x mod 512, y mod 512) and HOFS/VOFS are the camera mod 512.
// The HUD (BG0) has no block map. For the extra columns of a widescreen view we rebuild entries
// straight from the block map and table:
//
//   block = u16 at map + ((ty >> 2) * width + (tx >> 2)) * 2         (tx, ty) = room tile
//   entry = u16 at table + block * 32 + ((ty & 3) * 4 + (tx & 3)) * 2
//
// The game runs the next frame's logic before the frame is drawn, so the camera in RAM can be a
// few pixels ahead of HOFS/VOFS. The room position of the view is therefore taken as HOFS/VOFS
// plus the multiple of the VRAM map size that brings it closest to the camera in RAM.
//
// Once per frame sample tiles of the visible area are checked against VRAM. A layer that does
// not match (room transitions, the pause menu and map, cutscenes and other screens that draw
// their own maps) is hidden at the sides; only frames where at least one room layer matches
// widen. Everything else (the HUD and text boxes on BG0) is hidden at the sides rather than
// showing stale VRAM. Only addresses and layout descriptions belong here; never copy game data
// into the repo.
#include "gba.h"

namespace circlemoon::gba {

namespace {
constexpr u32 kLayers = 0x020230F0;  // 4 descriptors, BG0..BG3
constexpr u32 kLayerSize = 0x40;
constexpr u32 kCnt = 0x02, kTable = 0x04, kMap = 0x0C, kWidth = 0x10, kHeight = 0x12, kScreen = 0x14;
constexpr u32 kCamX = 0x18, kCamY = 0x1C;
constexpr s32 kMaxBlocks = 0x400;  // each block map buffer holds 1024 blocks

bool in_ewram(u32 a) { return (a >> 24) == 0x02; }
bool in_rom(u32 a) { return (a >> 24) >= 0x08 && (a >> 24) <= 0x09; }
}  // namespace

u16 Gba::wide_ram16(u32 a) const {
    if ((a >> 24) == 0x02) return u16(ewram_[a & 0x3FFFF] | (ewram_[(a + 1) & 0x3FFFF] << 8));
    if ((a >> 24) == 0x03) return u16(iwram_[a & 0x7FFF] | (iwram_[(a + 1) & 0x7FFF] << 8));
    if ((a >> 24) >= 0x08 && (a >> 24) <= 0x09) {
        const u32 off = a & 0x01FFFFFE;
        if (off + 1 < rom_.size()) return u16(rom_[off] | (rom_[off + 1] << 8));
    }
    return 0;
}

u32 Gba::wide_ram32(u32 a) const { return u32(wide_ram16(a)) | (u32(wide_ram16(a + 2)) << 16); }

u16 Gba::wide_level_entry(int bg, s32 tx, s32 ty) const {
    const WideLevel& L = wide_level_[bg];
    const u16 block = wide_ram16(L.map + u32((ty >> 2) * L.stride + (tx >> 2)) * 2);
    return wide_ram16(L.table + u32(block) * 32 + u32((ty & 3) * 4 + (tx & 3)) * 2);
}

void Gba::wide_level_detect() {
    for (auto& L : wide_level_) L = WideLevel{};
    wide_scene_ = false;
    wide_main_bg_ = -1;
    wide_hud_prio_ = -1;
    if (!wide_margin_) return;
    const u16 dispcnt = u16(io_[0] | (io_[1] << 8));
    if ((dispcnt & 7) != 0) return;  // rooms are drawn in mode 0

    int main_bg = -1;
    s32 main_area = 0;
    for (int layer = 1; layer < 4; ++layer) {  // layer 0 is the HUD
        const u32 d = kLayers + u32(layer) * kLayerSize;
        const u32 table = wide_ram32(d + kTable), map = wide_ram32(d + kMap);
        const s32 width = s32(wide_ram16(d + kWidth)), height = s32(wide_ram16(d + kHeight));
        if (!in_rom(table) || !in_ewram(map) || width <= 0 || height <= 0 || width * height > kMaxBlocks) continue;
        const u16 want = wide_ram16(d + kCnt);
        const u32 screen = wide_ram32(d + kScreen);
        int bg = -1;
        for (int b = 0; b < 4; ++b) {
            const u16 cnt = u16(io_[0x08 + 2 * b] | (io_[0x09 + 2 * b] << 8));
            if (!(dispcnt & (0x100 << b)) || ((cnt >> 8) & 31) != ((want >> 8) & 31)) continue;
            if (screen != 0x06000000u + ((cnt >> 8) & 31) * 0x800) continue;
            if (cnt & 0x80) continue;  // 16-color maps only
            bg = b;
            break;
        }
        if (bg < 0 || wide_level_[bg].mode == WideMode::Level) continue;

        const u16 cnt = u16(io_[0x08 + 2 * bg] | (io_[0x09 + 2 * bg] << 8));
        const u32 size = cnt >> 14;
        const s32 ring_w = (size & 1) ? 512 : 256, ring_h = (size & 2) ? 512 : 256;
        const s32 hofs = s32(u32(io_[0x10 + 4 * bg] | (io_[0x11 + 4 * bg] << 8)) & 0x1FF);
        const s32 vofs = s32(u32(io_[0x12 + 4 * bg] | (io_[0x13 + 4 * bg] << 8)) & 0x1FF);
        const s32 cam_x = s32(wide_ram32(d + kCamX)) >> 16, cam_y = s32(wide_ram32(d + kCamY)) >> 16;
        // HOFS/VOFS plus the multiple of the map size closest to the camera in RAM.
        auto nearest = [](s32 ofs, s32 cam, s32 ring) {
            const s32 t = cam - ofs + ring / 2;
            return (t >= 0 ? t / ring : -((ring - 1 - t) / ring)) * ring;
        };

        WideLevel& L = wide_level_[bg];
        L.layer = layer;
        L.map = map;
        L.table = table;
        L.stride = width;
        L.width = width;
        L.height = height;
        L.tiles_w = width * 4;
        L.tiles_h = height * 4;
        L.cam_x = nearest(hofs, cam_x, ring_w);
        L.cam_y = nearest(vofs, cam_y, ring_h);

        // Sample tiles spread over the visible area; at least 15 of every 16 must agree
        // (animated tiles and breakable walls can differ for a frame).
        const u32 screen_base = ((cnt >> 8) & 31) * 0x800;
        static constexpr int kSx[] = {4, 36, 68, 100, 132, 164, 196, 228};
        static constexpr int kSy[] = {12, 52, 92, 132, 156};
        int checked = 0, match = 0;
        for (int sy : kSy)
            for (int sx : kSx) {
                const s32 ax = sx + hofs + L.cam_x, ay = sy + vofs + L.cam_y;
                if (ax < 0 || ay < 0 || ax >= L.tiles_w * 8 || ay >= L.tiles_h * 8) continue;
                const u32 vx = u32(sx + hofs) & u32(ring_w - 1), vy = u32(sy + vofs) & u32(ring_h - 1);
                const u32 blk = (vx >> 8) + (vy >> 8) * ((size & 1) ? 2 : 1);
                const u32 off = screen_base + blk * 0x800 + ((vy & 255) >> 3) * 64 + ((vx & 255) >> 3) * 2;
                if (off + 1 >= vram_.size()) continue;
                ++checked;
                if (u16(vram_[off] | (vram_[off + 1] << 8)) == wide_level_entry(bg, ax >> 3, ay >> 3)) ++match;
            }
        if (checked < 8 || match * 16 < checked * 15) {
            L = WideLevel{};
            continue;
        }
        L.mode = WideMode::Level;
        if (width * height > main_area) {
            main_area = width * height;
            main_bg = bg;
        }
    }

    if (main_bg < 0) return;
    wide_scene_ = true;
    // Side columns past the room's edge are black; the largest room layer decides.
    wide_main_bg_ = main_bg;
    // The HUD is drawn on BG0, not with objects, so no object priority needs keeping in the view.
    wide_hud_prio_ = -1;
    // Everything that is not a matched room layer (the HUD and text boxes on BG0) would show
    // stale or wrapped VRAM: hide its sides.
    for (int bg = 0; bg < 4; ++bg)
        if (wide_level_[bg].mode != WideMode::Level) wide_level_[bg].mode = WideMode::Hide;
}

}  // namespace circlemoon::gba
