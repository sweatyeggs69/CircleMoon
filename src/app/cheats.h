#pragma once
// Cheats menu toggles (F1 > Cheats). After every game frame the host calls apply() on the game's
// EWRAM, which tops the chosen stats back up to Nathan's current maximums: HP, MP and hearts
// (subweapon ammo). Nothing is written unless the stats look like a loaded game (level and
// maximums in range), so the title and file select screens are left alone (the
// title screen's attract demo does load stats, so the cheats also top it up, which is harmless). Off while
// RetroAchievements hardcore mode is on. The cheats never raise a maximum.
//
// Settings (circlemoon::Config): cheats.hp, cheats.mp, cheats.hearts (all off by default).
//
// Unlocks (grant()) are one-time writes the user asks for from the menu after a confirmation:
// all DSS cards, all relics, every item, or maximum EXP (the game then levels Nathan up to 99). They stay in
// the game once it is saved, so the menu says so before applying them.
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

// True when EWRAM holds a loaded game: a level and every maximum in range.
inline bool game_loaded(std::span<const std::uint8_t> ewram) {
    using namespace detail;
    if (ewram.size() < off(game::kRelicsAddress) + game::kRelicCount) return false;
    const unsigned level = ewram[off(game::kLevelAddress)];
    const unsigned max_hp = get16(ewram, game::kMaxHpAddress), max_mp = get16(ewram, game::kMaxMpAddress),
                   max_hearts = get16(ewram, game::kMaxHeartsAddress);
    return level >= 1 && level <= kMaxLevel && max_hp >= 1 && max_hp <= kMaxStat && max_mp >= 1 &&
           max_mp <= kMaxStat && max_hearts <= kMaxStat;
}

// Applies the enabled cheats to ewram (the 256 KiB EWRAM, starting at 0x02000000). Returns true
// when anything changed.
inline bool apply(std::span<std::uint8_t> ewram, const Flags& f) {
    using namespace detail;
    if (!f.any() || !game_loaded(ewram)) return false;
    const unsigned max_hp = get16(ewram, game::kMaxHpAddress), max_mp = get16(ewram, game::kMaxMpAddress),
                   max_hearts = get16(ewram, game::kMaxHeartsAddress);
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

enum class Unlock { Cards, Relics, Items, MaxLevel };

// Applies one unlock to a loaded game. Returns false (and writes nothing) when no game is loaded.
// Owned cards and relics keep their value; only missing ones are set. Item counts below the
// stack size are raised to it.
inline bool grant(std::span<std::uint8_t> ewram, Unlock u) {
    using namespace detail;
    if (!game_loaded(ewram)) return false;
    auto own_all = [&](unsigned at, unsigned n) {
        for (unsigned i = 0; i < n; ++i)
            if (ewram[off(at) + i] == 0) ewram[off(at) + i] = 1;
    };
    switch (u) {
        case Unlock::Cards: own_all(game::kCardsAddress, game::kCardCount); break;
        case Unlock::Relics: own_all(game::kRelicsAddress, game::kRelicCount); break;
        case Unlock::Items:
            for (unsigned i = 0; i < game::kItemCount; ++i)
                if (ewram[off(game::kItemsAddress) + i] < game::kMaxItemStack)
                    ewram[off(game::kItemsAddress) + i] = std::uint8_t(game::kMaxItemStack);
            break;
        case Unlock::MaxLevel: {
            const std::size_t e = off(game::kExpAddress);
            const unsigned exp = ewram[e] | (ewram[e + 1] << 8) | (ewram[e + 2] << 16) | (unsigned(ewram[e + 3]) << 24);
            if (exp < game::kMaxExp)
                for (int i = 0; i < 4; ++i) ewram[e + i] = std::uint8_t(game::kMaxExp >> (8 * i));
            break;
        }
    }
    return true;
}

}  // namespace circlemoon::cheats
