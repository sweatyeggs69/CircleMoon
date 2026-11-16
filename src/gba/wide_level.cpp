// Widescreen support specific to Castlevania: Circle of the Moon (USA, AAME).
//
// Not mapped yet: the extra columns of a widescreen view have to be rebuilt from the room layout
// in RAM, and that layout (room tilemap, camera position) has not been located for this game.
// Until it is, game::kWidescreen is false, the host never sets a wide margin and every frame is
// the plain 3:2 picture. Only addresses and layout descriptions belong here; never copy game data
// into the repo.
#include "gba.h"

namespace circlemoon::gba {

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
    return wide_ram16(L.map + u32(ty * L.stride + tx) * 2);
}

void Gba::wide_level_detect() {
    for (auto& L : wide_level_) L = WideLevel{};
    wide_scene_ = false;
    wide_main_bg_ = -1;
    wide_hud_prio_ = -1;
}

}  // namespace circlemoon::gba
