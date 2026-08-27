#pragma once

#include "clib/u8g2.h"
#include <U8g2lib.h>

// custom elements

#define DISPENSE_WIDTH

namespace custom {

struct Dispense {
    unsigned long start = 0, duration = 0;
    u8g2_uint_t x, y;
    u8g2_uint_t fixed; // fixed side-length, width or height
    u8g2_uint_t max;

    constexpr Dispense(u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t fixed, u8g2_uint_t max) : x(x), y(y), fixed(fixed), max(max) { }


    void draw(U8G2& u8g2, unsigned long now) const {
        double elapsed = now - start;
        auto length  = static_cast<u8g2_uint_t>(elapsed / duration * max);
        u8g2.setDrawColor(1);
#ifdef DISPENSE_WIDTH
        u8g2.drawBox(x, y, length, fixed);
#else
        u8g2.drawBox(x, y, fixed, length);
#endif
    }

};

} // namespace custom

// custom element constructors

#define DISPENSE custom::Dispense(24, 50, 10, 80)
