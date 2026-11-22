#pragma once
// The one place that describes the game CircleMoon runs. Everything game-specific that the rest of
// the code needs (names shown to the user, ROM fingerprint, save layout) lives here, so the
// launcher, packaging and tests do not hard-code it.
//
// Target: Castlevania: Circle of the Moon, USA release (AGB-AAME), revision 0.
//
// Nothing here is game data: these are only identifiers and a fingerprint of the user's own ROM.
// RAM addresses come from the Data Crystal RAM map (datacrystal.tcrf.net) and still need checking
// in game.

#include <cstddef>
#include <string_view>

namespace circlemoon::game {

// Shown in the launcher, dialogs and error messages.
inline constexpr const char* kTitle = "Castlevania: Circle of the Moon";
inline constexpr const char* kRegion = "USA";
inline constexpr const char* kTitleWithRegion = "Castlevania: Circle of the Moon (USA)";
// Used for shortcuts, default export names, etc. (no characters that are invalid in file names).
inline constexpr const char* kFileSafeTitle = "Castlevania - Circle of the Moon";

// GBA cartridge header of the supported dump.
inline constexpr const char* kHeaderTitle = "DRACULA AGB1";  // 0xA0, 12 bytes
inline constexpr const char* kGameCode = "AAME";             // 0xAC, 4 bytes
inline constexpr const char* kMakerCode = "A4";             // 0xB0, 2 bytes (Konami)
inline constexpr unsigned kRevision = 0;                     // 0xBC
inline constexpr std::size_t kRomSize = 8 * 1024 * 1024;

// SHA-1 of the known-good dump (lowercase hex), checked against the maintainer's own cartridge
// dump on 2026-11-16 (MD5 50a1089600603a94e15ecf287f8d5a1f, CRC32 1CC059A4). If this is ever
// emptied, a ROM is accepted on a header match instead and the launcher labels it "header match
// (fingerprint not pinned yet)".
inline constexpr const char* kExpectedSha1 = "d661b01eb94186435723ac03344792c11c20c522";

// Other releases of the game, recognised only so the error message can say what the file is.
// None of them can run: the recompiled code is the USA revision 0 code.
struct OtherRelease {
    const char* sha1;       // exact dump, or nullptr to match on the game code alone
    const char* game_code;  // 0xAC
    const char* name;       // shown to the user
};
inline constexpr OtherRelease kOtherReleases[] = {
    {nullptr, "AAMP", "a European release"},
    {nullptr, "AAMJ", "a Japanese release"},
};

// Battery save: 256 Kbit (32 KiB) SRAM (the ROM carries the SRAM_V110 library string). The runtime
// picks the chip from that string; this is the size of the save file it writes.
inline constexpr std::size_t kSaveSize = 32 * 1024;
inline constexpr const char* kSaveChip = "SRAM";
// Bytes the game writes at the start of its save, used to reject saves from other games. The game
// writes "DRACULA AGB" and a closing zero at SRAM 0 on first boot, before any file is made
// (confirmed on 2026-11-16 against the save the game creates). It holds a zero byte, so the length
// is explicit.
inline constexpr std::string_view kSaveHeader{"DRACULA AGB\0", 12};

// Widescreen view (16:10 and 16:9). The extra columns are rebuilt from the room in RAM by
// src/gba/wide_level.cpp; false hides the option and keeps the picture 3:2.
inline constexpr bool kWidescreen = true;

// Nathan's stats (EWRAM), used by the cheats menu. All u16 unless noted.
//   0x0202562E HP   0x02025632 max HP   0x02025636 MP   0x0202563A max MP
//   0x0202563C hearts   0x0202563E max hearts   0x02025664 u8 level
inline constexpr unsigned kHpAddress = 0x0202562E;
inline constexpr unsigned kMaxHpAddress = 0x02025632;
inline constexpr unsigned kMpAddress = 0x02025636;
inline constexpr unsigned kMaxMpAddress = 0x0202563A;
inline constexpr unsigned kHeartsAddress = 0x0202563C;
inline constexpr unsigned kMaxHeartsAddress = 0x0202563E;
inline constexpr unsigned kLevelAddress = 0x02025664;

// One-time unlocks (F1 > Cheats > Unlocks). They change the loaded game for good, so the menu asks
// first. Addresses from Data Crystal and the Code Breaker codes for this release.
//   0x02025668 u32 EXP. At 99,999,999 the game levels Nathan up one level about every two
//              seconds (with the LEVEL UP banner) until level 99, about three minutes.
//   0x02025674 20 bytes, one per DSS card (10 attribute cards Salamander..Black Dog, then 10
//              action cards Mercury..Pluto); nonzero = owned. Checked in game 2026-11-16.
//   0x020256ED 54 bytes, one count per item (body wear, then accessories, then consumables).
//              The Max Up counters follow at 0x0202572C, so the range must stay 54 bytes.
//   0x0202572F 8 bytes, one per relic (Dash Boots, Double, Tackle, Kick Boots, Heavy Ring,
//              Cleansing, Roc Wing, Last Key); nonzero = owned.
inline constexpr unsigned kExpAddress = 0x02025668;
inline constexpr unsigned kMaxExp = 99999999;
inline constexpr unsigned kCardsAddress = 0x02025674;
inline constexpr unsigned kCardCount = 20;
inline constexpr unsigned kItemsAddress = 0x020256ED;
inline constexpr unsigned kItemCount = 54;
inline constexpr unsigned kMaxItemStack = 99;
inline constexpr unsigned kRelicsAddress = 0x0202572F;
inline constexpr unsigned kRelicCount = 8;

}  // namespace circlemoon::game
