// Cheats: refills Nathan's HP, MP and hearts in a blank EWRAM image. No ROM needed.
#include <cstdint>
#include <cstdio>
#include <vector>

#include "app/cheats.h"

namespace {
int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
using circlemoon::game::kHpAddress, circlemoon::game::kMaxHpAddress, circlemoon::game::kMpAddress,
    circlemoon::game::kMaxMpAddress, circlemoon::game::kHeartsAddress, circlemoon::game::kMaxHeartsAddress,
    circlemoon::game::kLevelAddress;
std::size_t at(unsigned a) { return a - 0x02000000; }
std::uint16_t get16(const std::vector<std::uint8_t>& m, unsigned a) { return std::uint16_t(m[at(a)] | (m[at(a) + 1] << 8)); }
void set16(std::vector<std::uint8_t>& m, unsigned a, std::uint16_t v) { m[at(a)] = std::uint8_t(v); m[at(a) + 1] = std::uint8_t(v >> 8); }
}  // namespace

int main() {
    namespace cheats = circlemoon::cheats;
    using cheats::Flags;
    std::vector<std::uint8_t> ram(0x40000, 0);
    const Flags all{true, true, true};

    // Nothing enabled, or no game loaded (level 0): no writes.
    CHECK(!cheats::apply(ram, Flags{}));
    CHECK(!cheats::apply(ram, all));

    // A new game: level 1, 80/100 HP, 20/30 MP, 3/10 hearts.
    ram[at(kLevelAddress)] = 1;
    set16(ram, kMaxHpAddress, 100);
    set16(ram, kHpAddress, 80);
    set16(ram, kMaxMpAddress, 30);
    set16(ram, kMpAddress, 20);
    set16(ram, kMaxHeartsAddress, 10);
    set16(ram, kHeartsAddress, 3);

    CHECK(cheats::apply(ram, Flags{true, false, false}));
    CHECK(get16(ram, kHpAddress) == 100 && get16(ram, kMaxHpAddress) == 100);  // max never changes
    CHECK(get16(ram, kMpAddress) == 20 && get16(ram, kHeartsAddress) == 3);
    CHECK(!cheats::apply(ram, Flags{true, false, false}));  // already full

    CHECK(cheats::apply(ram, Flags{false, true, true}));
    CHECK(get16(ram, kMpAddress) == 30 && get16(ram, kHeartsAddress) == 10);

    // Garbage (not a loaded game): no writes.
    set16(ram, kHpAddress, 1);
    ram[at(kLevelAddress)] = 200;
    CHECK(!cheats::apply(ram, all));
    ram[at(kLevelAddress)] = 5;
    set16(ram, kMaxHpAddress, 0);
    CHECK(!cheats::apply(ram, all));
    set16(ram, kMaxHpAddress, 60000);
    CHECK(!cheats::apply(ram, all));
    CHECK(get16(ram, kHpAddress) == 1);

    if (failures) std::fprintf(stderr, "%d check(s) failed\n", failures);
    else std::printf("cheats tests passed\n");
    return failures ? 1 : 0;
}
