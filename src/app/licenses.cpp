#include "app/licenses.h"

#include <cstdio>

extern const unsigned char circlemoon_text_license[];
extern const unsigned circlemoon_text_license_size;
extern const unsigned char circlemoon_text_notices[];
extern const unsigned circlemoon_text_notices_size;
extern const unsigned char circlemoon_text_dmsans_ofl[];
extern const unsigned circlemoon_text_dmsans_ofl_size;

namespace circlemoon::licenses {

namespace {
std::string_view view(const unsigned char* p, unsigned n) { return {reinterpret_cast<const char*>(p), n}; }
}  // namespace

std::string_view circlemoon() { return view(circlemoon_text_license, circlemoon_text_license_size); }
std::string_view notices() { return view(circlemoon_text_notices, circlemoon_text_notices_size); }
std::string_view font_ofl() { return view(circlemoon_text_dmsans_ofl, circlemoon_text_dmsans_ofl_size); }

void print_all() {
    for (std::string_view t : {circlemoon(), notices(), font_ofl()}) {
        std::fwrite(t.data(), 1, t.size(), stdout);
        std::fputs("\n\n", stdout);
    }
}

}  // namespace circlemoon::licenses
