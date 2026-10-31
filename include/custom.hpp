#pragma once

#include "esp32-hal-adc.h"
#include <U8g2lib.h>

#include "logging.hpp"

// custom elements

#define DISPENSE_WIDTH

#define MOTOR_OUT_1 36
#define MOTOR_OUT_2 39

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

struct Sense {
    unsigned long start = 0, duration = 0;
    u8g2_uint_t x, y;

    constexpr Sense(u8g2_uint_t x, u8g2_uint_t y) : x(x), y(y) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        const auto syrup_motor_voltage = analogReadMilliVolts(MOTOR_OUT_1) / 1000.0; // ADC over R1
        const auto soda_motor_voltage = analogReadMilliVolts(MOTOR_OUT_2) / 1000.0; // ADC over R2
        const auto syrup_motor_current = syrup_motor_voltage / 0.63;
        const auto soda_motor_current = soda_motor_voltage / 0.57;
        //LOG_DEBUG("sense", "syrup motor: %.2lf V, %.2lf A", syrup_motor_voltage, syrup_motor_current);
        char buffer[32];
        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_fourmat_tf);
        u8g2.setFontMode(1);
        std::snprintf(buffer, 32, "SYRUP MOTOR (%.2lfA)", syrup_motor_current);
        u8g2.drawStr(x, y, buffer);
        std::snprintf(buffer, 32, "SODA MOTOR (%.2lfA)", soda_motor_current);
        u8g2.drawStr(x, y - 6, buffer);
    }
};

} // namespace custom

// custom element constructors

#define DISPENSE custom::Dispense(24, 50, 10, 80)

#define SENSE custom::Sense(2, 38)
