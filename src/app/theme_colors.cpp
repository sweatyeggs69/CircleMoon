// CircleMoon's colors for the shared launcher theme (theme.h): near-black night surfaces with a
// wine tint, a blood-crimson accent after Dracula's castle, and a pale moonlight-gold focus ring
// for the circle of the moon. This is the only theme file that differs between ports.
#include "app/theme.h"

namespace circlemoon::theme {

namespace {
ImVec4 rgb(int r, int g, int b, float a = 1.0f) { return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a); }
}  // namespace

const Palette& palette() {
    static const Palette p{
        rgb(17, 12, 18),            // surface
        rgb(25, 18, 27),            // card
        rgb(35, 25, 38),            // raised
        rgb(46, 33, 50),            // high
        rgb(230, 200, 220, 0.14f),  // line
        rgb(248, 242, 246),         // ink
        rgb(208, 190, 202),         // ink_muted
        rgb(150, 132, 146),         // ink_faint
        rgb(196, 38, 58),           // accent (crimson)
        rgb(220, 64, 84),           // accent_hover
        rgb(164, 26, 44),           // accent_active
        rgb(196, 38, 58, 0.18f),    // accent_soft
        rgb(255, 244, 246),         // on_accent
        rgb(236, 220, 160),         // highlight (moonlight gold)
        rgb(52, 211, 153),          // good
        rgb(248, 113, 113),         // bad
        rgb(120, 20, 36),           // glow
    };
    return p;
}

}  // namespace circlemoon::theme
