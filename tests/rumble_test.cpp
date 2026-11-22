// Rumble: which changes in a blank EWRAM image turn into controller rumble patterns. No ROM needed.
#include <array>
#include <cstdint>
#include <cstdio>

#include "app/rumble.h"

namespace {
int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
using Ram = std::array<std::uint8_t, 0x40000>;
std::size_t ew(unsigned a) { return a - 0x02000000u; }
void set16(Ram& m, unsigned a, std::uint16_t v) { m[ew(a)] = std::uint8_t(v); m[ew(a) + 1] = std::uint8_t(v >> 8); }
}  // namespace

int main() {
    namespace rumble = circlemoon::rumble;
    namespace game = circlemoon::game;
    const std::array<std::uint8_t, 0x8000> iwram{};
    Ram ram{};
    rumble::Watcher w;

    // Title screen: nothing loaded, nothing rumbles.
    CHECK(!w.step(iwram, ram));
    CHECK(!w.step(iwram, ram));

    // A loaded game: level 10, 300/300 HP, 100 MP, 50 hearts.
    ram[ew(game::kLevelAddress)] = 10;
    set16(ram, game::kMaxHpAddress, 300);
    set16(ram, game::kHpAddress, 300);
    set16(ram, game::kMaxMpAddress, 100);
    set16(ram, game::kMaxHeartsAddress, 50);
    CHECK(!w.step(iwram, ram));  // the game just loaded
    CHECK(!w.step(iwram, ram));

    // A scratch: a light buzz, one step.
    set16(ram, game::kHpAddress, 290);
    rumble::Pattern p = w.step(iwram, ram);
    CHECK(p && p.count == 1 && p.strength() < 0.8f);

    // A big hit: a heavy thump, stronger.
    set16(ram, game::kHpAddress, 150);
    rumble::Pattern big = w.step(iwram, ram);
    CHECK(big && big.count == 2 && big.strength() > p.strength());

    // Potions and cheats refilling never rumble.
    set16(ram, game::kHpAddress, 300);
    CHECK(!w.step(iwram, ram));

    // A level up raises the maximum: not a hit.
    set16(ram, game::kMaxHpAddress, 320);
    set16(ram, game::kHpAddress, 200);
    CHECK(!w.step(iwram, ram));

    // Death: the long pattern.
    set16(ram, game::kHpAddress, 0);
    p = w.step(iwram, ram);
    CHECK(p && p.priority == rumble::patterns::kDeathPriority && p.total_ms() > 1000);

    // reset(): a loaded state with less HP is not a hit.
    set16(ram, game::kHpAddress, 300);
    w.step(iwram, ram);
    w.reset();
    set16(ram, game::kHpAddress, 20);
    CHECK(!w.step(iwram, ram));

    // Player: steps through a pattern a frame at a time, and a lower priority pattern never cuts a
    // higher one short; it plays again once that is over.
    {
        rumble::Player pl;
        CHECK(pl.start(rumble::patterns::kDeath));
        auto s = pl.next();
        CHECK(s && s->low == 1.0f && s->high == 1.0f && s->ms > 250);  // first beat, with overlap
        CHECK(!pl.start(rumble::patterns::kMissile));
        int commands = 1, frames = 0;
        bool paused = false;
        while (pl.playing() && frames < 200) {
            if (auto c = pl.next()) {
                ++commands;
                if (c->low == 0 && c->high == 0 && pl.playing()) paused = true;
            }
            ++frames;
        }
        CHECK(paused);                          // the pauses between beats stop the motors
        CHECK(commands == 7);                   // six steps, then the final stop
        CHECK(frames >= 75 && frames <= 90);    // about 1.4 s of game frames
        CHECK(!pl.playing());
        CHECK(pl.start(rumble::patterns::kMissile));
        CHECK(pl.start(rumble::patterns::kDeath));  // a death interrupts anything
        CHECK(!pl.start({}));
    }

    // The pattern table: different events feel different.
    {
        namespace pt = rumble::patterns;
        CHECK(pt::kMissile.peak_low() == 0 && pt::kMissile.peak_high() > 0);  // small motor only
        CHECK(pt::kDeath.total_ms() > pt::kHeavyHit.total_ms());
        CHECK(pt::kPowerBomb.strength() == 1.0f && pt::kPowerBomb.steps[0].low < 0.5f);  // builds up
        const rumble::Pattern scratch = rumble::hit(1, 100), big = rumble::hit(40, 100);
        CHECK(scratch.count == 1 && big.count == 2 && big.strength() > scratch.strength());
        const rumble::Pattern sh = rumble::shake(600, 0.8f);
        CHECK(sh.count > 4 && sh.total_ms() == 600 && sh.priority == pt::kShakePriority);
        CHECK(rumble::shake(5000, 1.0f).total_ms() == 1200);
    }

    // Strength scaling.
    CHECK(rumble::motor(1.0f, 100) == 0xFFFF);
    CHECK(rumble::motor(1.0f, 50) == 0x8000);
    CHECK(rumble::motor(0.0f, 100) == 0);
    CHECK(rumble::motor(2.0f, 100) == 0xFFFF);

    // Strength scaling.
    CHECK(rumble::motor(1.0f, 100) == 0xFFFF);
    CHECK(rumble::motor(1.0f, 50) == 0x8000);
    CHECK(rumble::motor(0.0f, 100) == 0);

    if (failures) return 1;
    std::puts("rumble: ok");
    return 0;
}
