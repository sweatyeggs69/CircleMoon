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
inline constexpr unsigned kRevision = 0;                     // 0xBC
inline constexpr std::size_t kRomSize = 8 * 1024 * 1024;

// SHA-1 of the known-good dump (lowercase hex). Not pinned yet: it gets filled in from the
// maintainer's own cartridge dump (CRC32 1CC059A4 per Data Crystal). While it is empty, a ROM is
// accepted on a header match and the launcher labels it "header match (fingerprint not pinned yet)".
inline constexpr const char* kExpectedSha1 = "";

// Battery save. The runtime picks the chip from the library string in the ROM; this is the size of
// the save file it writes. To be confirmed against the ROM (TODO before release).
inline constexpr std::size_t kSaveSize = 32 * 1024;
inline constexpr const char* kSaveChip = "SRAM";
// Bytes the game writes at the start of its save, used to reject saves from other games. Empty
// disables the check until the signature is read from a real save.
inline constexpr std::string_view kSaveHeader{};

// Widescreen view (16:10 and 16:9). The extra columns need the room layout in RAM, which is not
// mapped for this game yet (src/gba/wide_level.cpp), so the option stays hidden and the picture 3:2.
inline constexpr bool kWidescreen = false;

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

}  // namespace circlemoon::game
