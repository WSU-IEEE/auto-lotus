// alternative build target to calibrate constants on real hardware
#include <Wire.h>
#include "util.hpp"

#include "esp32-hal-gpio.h"
#include "util.hpp"
#include "scheduler.hpp"
#include "input.hpp"

#define MOTOR_START 10000LU
#define MOTOR_END   105005U

constexpr auto time_start = 10000LU;
constexpr auto time_end =  time_start + 5000LU;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

Scheduler scheduler;

volatile bool start = false;
volatile bool end = false;

input::ButtonInput input_provider(lotus::constants::pins_btn);

template <size_t M>
void enable_motor() {
    lotus::set_motors<M, 1>({HIGH});
}

void disable_motor() {
    lotus::set_motors<0, 3>({LOW, LOW, LOW});
}

void setup() {
    Serial.begin(115200); // baud rate
    Wire.begin(lotus::constants::pin_I2C_sda, lotus::constants::pin_I2C_scl);
    u8g2.begin();
    u8g2.setFontPosTop();
    u8g2.setFontMode(1);
    u8g2.setFont(u8g2_font_9x15_tf);

    // outputs to control the L298N motor driver
    lotus::set_modes<OUTPUT>(lotus::constants::pins_motor);
    input_provider.setup();

    /*
    scheduler.schedule(
        MOTOR_START,
        []() {
            enable_motor();
        }
    );
    scheduler.schedule(
        MOTOR_END,
        []() {
            disable_motor();
        }
    );
    */

    scheduler.schedule(
        time_start,
        [](unsigned long) {
            start = true;
        }
    );
    scheduler.schedule(
        time_end,
        [](unsigned long) {
            end = true;
        }
    );

    Serial.println("setup complete!");
}

void loop() {
    const auto now = millis();

    scheduler.process(now);

    u8g2.clearBuffer();

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02lu.%03lu", now / 1000, now % 1000);
    u8g2.drawStr(16, 26, buffer);

    if (start)
        u8g2.drawBox(96, 24, 8, 8);
    else
        u8g2.drawFrame(96, 24, 8, 8);
    if (end)
        u8g2.drawBox(96, 32, 8, 8);
    else
        u8g2.drawFrame(96, 32, 8, 8);

    Serial.println(input_provider.poll(now) ? "BUTTON PRESSED!" : "NOTHING PRESSED.");
    auto pressed = input_provider.get();
    for (size_t i = 0; i < 4; ++i) {
        Serial.print(pressed[i] == input::Type::Neutral ? "NEUTRAL" : pressed[i] == input::Type::Pressed ? "PRESSED" : "STALE");
        Serial.printf("(%d), ", digitalRead(lotus::constants::pins_btn[i]));
        if (pressed[i] == input::Type::Pressed)
            u8g2.drawBox(32 + i * 10, 50, 8, 8);
        else if (pressed[i] == input::Type::Stale) {
            u8g2.drawFrame(32 + i * 10, 50, 8, 8);
            u8g2.drawBox(34 + i * 10, 52, 4, 4);
        } else
            u8g2.drawFrame(32 + i * 10, 50, 8, 8);
    }
    Serial.print("\n");

    if (pressed[0] == input::Type::Pressed && pressed[2] != input::Type::Pressed)
        enable_motor<0>();
    else if (pressed[1] == input::Type::Pressed && pressed[2] != input::Type::Pressed)
        enable_motor<1>();
    else if (pressed[2] == input::Type::Pressed && pressed[0] != input::Type::Pressed)
        disable_motor();

    u8g2.sendBuffer();

    //delay(500);
}
