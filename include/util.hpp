#pragma once

#include <Arduino.h>

// flags

// #define DISABLE_POWER_SAVING

// end flags

// debug

// #define DEBUG_DISPENSE_TIME 5000LU

// end debug

#define OZ_TO_ML 29.57353F
#define MOTOR_PINS 4

// flow rate in ml/min
#define FLOW_RATE 408.0F

namespace constants {

// button pins
constexpr uint8_t pins_btn[3] = {
    19, // left
    18, // middle
    17  // right
};

// motor controller pins
constexpr uint8_t pins_motor[MOTOR_PINS] = {
    16, // IN1
    15, // IN2
    14, // IN3
    12  // IN4
};

// I2C pins for ESP32
constexpr int pin_I2C_sda = 21;
constexpr int pin_I2C_scl = 22;

// button debounce time
constexpr unsigned long debounce_delay = 20;

constexpr unsigned long timeout = 20000; // time (ms) until giving up and returning to sleep
constexpr unsigned long sleep_timeout = 80000; // time (ms) until turning off screen to save power
constexpr unsigned long leaving_timeout = 5000; // time (ms) "thank you" scene is displayed

// amount (ml) to dispense, maps selection index to oz
constexpr double dispense_amount[3] = {
    4.0 * OZ_TO_ML,
    8.0 * OZ_TO_ML,
    16.0 * OZ_TO_ML
};

// ratio of soda in mixed drink, maps selection index
// i.e. 0.6 is 6 parts soda to 4 parts lotus
constexpr double soda_ratio[3] = {
    0.9,
    0.8,
    0.7
};

// https://www.directindustry.com/prod/kamoer-fluid-tech-shanghai-co-ltd/product-242598-2511427.html
// in ml/ms, will probably need to be tuned
constexpr double flow_rate = FLOW_RATE / (60 * 1000);

}; // namespace constants

template <uint8_t M, size_t N>
static void setModes(const uint8_t (&pins)[N]) {
    for (size_t i = 0; i < N; i++) pinMode(pins[i], M);
}

template <size_t Start, size_t Count>
static void setMotors(const uint8_t (&state)[Count]) {
    for (size_t i = Start; i < Count + Start; i++) digitalWrite(constants::pins_motor[i], state[i]);
}
