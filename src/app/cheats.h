#pragma once
// Cheats menu toggles (F1 > Cheats). After every game frame the host calls apply() on the game's
// EWRAM, which tops the chosen stats back up to Nathan's current maximums: HP, MP and hearts
// (subweapon ammo). Nothing is written unless the stats look like a loaded game (level and
// maximums in range), so the title and file select screens are left alone (the
// title screen's attract demo does load stats, so the cheats also top it up, which is harmless). Off while
// RetroAchievements hardcore mode is on. The cheats never raise a maximum.
//
// Settings (circlemoon::Config): cheats.hp, cheats.mp, cheats.hearts (all off by default).
#include <cstddef>
#include <cstdint>
#include <span>

#include "game.h"

namespace circlemoon::cheats {

struct Flags {
    bool hp = false;
    bool mp = false;
    bool hearts = false;
    bool any() const { return hp || mp || hearts; }
};

namespace detail {
inline constexpr std::uint32_t kEwram = 0x02000000;
inline constexpr unsigned kMaxStat = 9999;
inline constexpr unsigned kMaxLevel = 99;
inline std::size_t off(unsigned address) { return address - kEwram; }
inline unsigned get16(std::span<const std::uint8_t> m, unsigned a) { return m[off(a)] | (m[off(a) + 1] << 8); }
}  // namespace detail

// Applies the enabled cheats to ewram (the 256 KiB EWRAM, starting at 0x02000000). Returns true
// when anything changed.
inline bool apply(std::span<std::uint8_t> ewram, const Flags& f) {
    using namespace detail;
    if (!f.any() || ewram.size() < off(game::kLevelAddress) + 1) return false;
    const unsigned level = ewram[off(game::kLevelAddress)];
    const unsigned max_hp = get16(ewram, game::kMaxHpAddress), max_mp = get16(ewram, game::kMaxMpAddress),
                   max_hearts = get16(ewram, game::kMaxHeartsAddress);
    // A loaded game has a level and every maximum in range.
    if (level < 1 || level > kMaxLevel) return false;
    if (max_hp < 1 || max_hp > kMaxStat || max_mp < 1 || max_mp > kMaxStat || max_hearts > kMaxStat) return false;
    bool changed = false;
    auto fill = [&](unsigned at, unsigned max) {
        if (get16(ewram, at) != max) {
            ewram[off(at)] = std::uint8_t(max);
            ewram[off(at) + 1] = std::uint8_t(max >> 8);
            changed = true;
        }
    };
    if (f.hp) fill(game::kHpAddress, max_hp);
    if (f.mp) fill(game::kMpAddress, max_mp);
    if (f.hearts) fill(game::kHeartsAddress, max_hearts);
    return changed;
}

}  // namespace circlemoon::cheats
