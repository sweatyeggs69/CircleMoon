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
// Past the room's left and right edges (the room has nothing there, and one-screen rooms are
// common) the extra columns mirror the room's own edge instead of showing black bars.
//
// Once per frame sample tiles of the visible area are checked against VRAM. A layer that does
// not match (room transitions, the pause menu and map, cutscenes and other screens that draw
// their own maps) is hidden at the sides; only frames where at least one room layer matches
// widen. Everything else (the HUD and text boxes on BG0) is hidden at the sides rather than
// showing stale VRAM. Only addresses and layout descriptions belong here; never copy game data
// into the repo.
#include "gba.h"

#include <algorithm>
#include <iterator>

namespace circlemoon::gba {

namespace {
constexpr u32 kLayers = 0x020230F0;  // 4 descriptors, BG0..BG3
constexpr u32 kLayerSize = 0x40;
constexpr u32 kCnt = 0x02, kTable = 0x04, kMap = 0x0C, kWidth = 0x10, kHeight = 0x12, kScreen = 0x14;
constexpr u32 kCamX = 0x18, kCamY = 0x1C;
constexpr s32 kMaxBlocks = 0x400;  // each block map buffer holds 1024 blocks
// Many rooms are a single screen wide (256 pixels), so a wide view shows past the room's left or
// right edge, where the room has nothing. Instead of black bars, those columns mirror the room's
// own edge (walls look like walls). The level rectangle is widened by this many tiles per side.
constexpr s32 kPadTiles = 4;
// Camera limits, one 4-byte entry per room layer (layer 1 first): +0 u8 flags (bit 0: the camera
// scrolls horizontally, otherwise it stays at x = 0), +1 u8 right limit in 256-pixel units (the
// camera stops at limit * 256 - 240). Rooms are built from whole blocks, so a one-screen room is
// 256 pixels wide but only its left 240 are ever shown; the rest is filler.
constexpr u32 kCameraLimits = 0x020278A0;

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
    // Columns past the room's left or right edge mirror the room's own edge (see kPadTiles).
    tx -= kPadTiles;
    const s32 room_w = L.width;  // tiles the camera can show
    bool mirror = false;
    if (tx < 0) tx = -1 - tx, mirror = true;
    else if (tx >= room_w) tx = 2 * room_w - 1 - tx, mirror = true;
    if (tx < 0 || tx >= room_w) return 0;
    const u16 block = wide_ram16(L.map + u32((ty >> 2) * L.stride + (tx >> 2)) * 2);
    const u16 e = wide_ram16(L.table + u32(block) * 32 + u32((ty & 3) * 4 + (tx & 3)) * 2);
    return mirror ? u16(e ^ 0x400) : e;
}

// Sprite on-screen check. The game builds OAM in code it copies to IWRAM at boot (from ROM
// 0x080C39A8 to 0x03000000). For each sprite piece it skips the piece unless it overlaps the
// 240x160 screen:
//
//   original                      patched (m = margin)
//   0300069C lsls r0, r4, #1      lsls r0, r4, #1
//   0300069E adds r0, r3, r0      adds r0, r3, r0          x + width
//   030006A0 cmp  r0, #0          adds r0, #m              x + width + m (sets the flags)
//   030006A2 ble  skip            ble  skip
//   030006A4 cmp  r3, #0xEF       cmp  r3, #(239 + m)      m <= 16, then nop
//   030006A6 bgt  skip            subs r0, r3, #(m - 16)   m > 16, then cmp r0, #0xFF
//   030006A8 lsls r0, r5, #1      (nop / cmp r0, #0xFF)
//   030006AA adds r0, r2, r0      bgt  skip
//   030006AC cmp  r0, #0          lsls r0, r5, #1
//   030006AE ble  skip            adds r0, r2, r0          y + height (sets the flags)
//   030006B0 cmp  r2, #0x9F       ble  skip ...            (unchanged from here)
//   030006B2 bgt  skip
//
// so pieces up to m pixels past either side are drawn into the extra columns instead of popping
// in at the old edge. The IWRAM copy is checked every frame (save states and resets bring back
// whichever form they held) and only rewritten when it is in one of the known forms.
void Gba::wide_sprite_patch(int margin) {
    static constexpr u32 kAt = 0x69C;  // IWRAM offset
    static constexpr u16 kOriginal[] = {0x0060, 0x1818, 0x2800, 0xDD1C, 0x2BEF, 0xDC1A,
                                        0x0068, 0x1810, 0x2800, 0xDD16, 0x2A9F, 0xDC14};
    constexpr std::size_t n = std::size(kOriginal);
    auto at = [&](std::size_t i) { return u16(iwram_[kAt + 2 * i] | (iwram_[kAt + 2 * i + 1] << 8)); };
    auto form = [](int m, std::size_t i) -> u16 {
        if (m <= 0) return kOriginal[i];
        static constexpr u16 kFixed[] = {0x0060, 0x1818, 0, 0xDD1C, 0, 0, 0xDC19, 0x0068, 0x1810, 0xDD16, 0x2A9F, 0xDC14};
        if (i == 2) return u16(0x3000 | m);                                         // adds r0, #m
        if (i == 4) return m <= 16 ? u16(0x2B00 | (239 + m)) : u16(0x1E18 | ((m - 16) << 6));  // cmp r3 / subs r0, r3
        if (i == 5) return m <= 16 ? u16(0x46C0) : u16(0x28FF);                     // nop / cmp r0, #0xFF
        return kFixed[i];
    };
    // Which form is in IWRAM now: the original, or patched for some margin.
    int now = -1;
    for (int m = 0; m <= kMaxWideMargin && now < 0; ++m) {
        bool same = true;
        for (std::size_t i = 0; i < n && same; ++i) same = at(i) == form(m, i);
        if (same) now = m;
    }
    if (now < 0 || now == margin) return;
    for (std::size_t i = 0; i < n; ++i) {
        const u16 v = form(margin, i);
        iwram_[kAt + 2 * i] = u8(v);
        iwram_[kAt + 2 * i + 1] = u8(v >> 8);
    }
}

void Gba::wide_level_detect() {
    wide_sprite_patch(wide_sprites_ ? wide_margin_ : 0);
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
        L.height = height;
        // How far right the camera can go: camera limits per layer at kCameraLimits.
        const u32 lim = kCameraLimits + u32(layer - 1) * 4;
        const s32 view_w = (wide_ram16(lim) & 1) ? s32(wide_ram16(lim) >> 8) * 256 : kScreenWidth;
        L.width = std::clamp(view_w, s32(kScreenWidth), width * 32) / 8;
        L.tiles_w = L.width + 2 * kPadTiles;
        L.tiles_h = height * 4;
        L.cam_x = nearest(hofs, cam_x, ring_w) + kPadTiles * 8;
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
                if (ax < kPadTiles * 8 || ay < 0 || ax >= (L.tiles_w - kPadTiles) * 8 || ay >= L.tiles_h * 8) continue;
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
    // Side columns above or below the room are black; the largest room layer decides. (Past its
    // left and right edges the room is mirrored, so those columns are inside the padded level.)
    wide_main_bg_ = main_bg;
    // The HUD is drawn on BG0, not with objects, so no object priority needs keeping in the view.
    wide_hud_prio_ = -1;
    // Everything that is not a matched room layer (the HUD and text boxes on BG0) would show
    // stale or wrapped VRAM: hide its sides.
    for (int bg = 0; bg < 4; ++bg)
        if (wide_level_[bg].mode != WideMode::Level) wide_level_[bg].mode = WideMode::Hide;
}

}  // namespace circlemoon::gba
