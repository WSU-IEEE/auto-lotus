// alternative build target to calibrate constants on real hardware
#include <Wire.h>

#include "util.hpp"
#include "scheduler.hpp"

#define RESET_MOTORS setMotors<0, 4>({LOW, LOW, LOW, LOW});

#define MOTOR_START 10000LU
#define MOTOR_END   15000LU

constexpr auto time_start = 10000LU;
constexpr auto time_end =  time_start + 5000LU;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

Scheduler scheduler;

volatile bool start = false;
volatile bool end = false;

void enableMotor() {
    // OUT3 and OUT4 are swapped on motor driver board so for motor two, REVERSE and FORWARD are flipped, i.e. LOW, HIGH should start REVERSE but instead it does forward, it is inverted here to counter that
    setMotors<0, 4>({LOW, HIGH, HIGH, LOW});
}

void setup() {
    Serial.begin(115200); // baud rate
    Wire.begin(constants::pin_I2C_sda, constants::pin_I2C_scl);
    u8g2.begin();
    u8g2.setFontPosTop();
    u8g2.setFontMode(1);
    u8g2.setFont(u8g2_font_9x15_tf);

    // outputs to control the L298N motor driver (assuming IN1, IN2 for motor 1 and IN3, IN4 for motor 2)
    setModes<OUTPUT>(constants::pins_motor);

    scheduler.schedule(MOTOR_START, [](){ enableMotor(); });
    scheduler.schedule(MOTOR_END, [](){ RESET_MOTORS });

    scheduler.schedule(time_start, [](){ start = true; });
    scheduler.schedule(time_end, [](){ end = true; });

    Serial.println("setup complete!");
}

void loop() {
    scheduler.process();

    u8g2.clearBuffer();

    auto now = millis();
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02lu.%03lu", now / 1000, now % 1000);
    u8g2.drawStr(16, 26, buffer);

    if (start) u8g2.drawBox(96, 24, 8, 8);
    else u8g2.drawFrame(96, 24, 8, 8);
    if (end) u8g2.drawBox(96, 32, 8, 8);
    else u8g2.drawFrame(96, 32, 8, 8);

    u8g2.sendBuffer();

    delay(20);
}
