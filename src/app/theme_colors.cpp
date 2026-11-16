// CircleMoon's colors for the shared launcher theme (theme.h): deep maroon surfaces, a bright
// crimson accent after Dracula's castle, and a pale moonlight-gold focus ring for the circle of
// the moon. This is the only theme file that differs between ports.
#include "app/theme.h"

namespace circlemoon::theme {

namespace {
ImVec4 rgb(int r, int g, int b, float a = 1.0f) { return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a); }
}  // namespace

const Palette& palette() {
    static const Palette p{
        rgb(46, 10, 18),            // surface (maroon)
        rgb(60, 15, 25),            // card
        rgb(76, 21, 33),            // raised
        rgb(94, 28, 42),            // high
        rgb(255, 205, 215, 0.16f),  // line
        rgb(252, 244, 246),         // ink
        rgb(222, 196, 204),         // ink_muted
        rgb(170, 134, 146),         // ink_faint
        rgb(214, 46, 66),           // accent (crimson)
        rgb(232, 74, 92),           // accent_hover
        rgb(180, 32, 50),           // accent_active
        rgb(214, 46, 66, 0.20f),    // accent_soft
        rgb(255, 244, 246),         // on_accent
        rgb(236, 220, 160),         // highlight (moonlight gold)
        rgb(52, 211, 153),          // good
        rgb(248, 113, 113),         // bad
        rgb(150, 30, 48),           // glow
    };
    return p;
}

}  // namespace circlemoon::theme
