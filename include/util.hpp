#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include <cstdint>

// flags

// #define DISABLE_POWER_SAVING

// end flags

// debug

// #define DEBUG_DISPENSE_TIME 5000LU

// end debug

#define OZ_TO_ML 29.57353F

// flow rate in ml/min
#define FLOW_RATE 408.0F

// index for motor
#define SYRUP_MOTOR 0U
#define SODA_MOTOR 1U

namespace lotus {

namespace constants {

constexpr unsigned int motor_pins = 3;
constexpr unsigned int button_pins = 4;

// button pins
constexpr std::array<uint8_t, button_pins> pins_btn{
    23,
    22,
    21,
    19
};

// motor controller pins
constexpr std::array<uint8_t, motor_pins> pins_motor{
    18, // motor 1, syrup
    17, // motor 2, soda
    15, // motor 3
};

// I2C pins for ESP32
constexpr int pin_I2C_sda = 25;
constexpr int pin_I2C_scl = 26;

// button debounce time
constexpr unsigned long debounce_delay = 20;

constexpr unsigned long timeout = 20000;          // time (ms) until giving up and returning to sleep
constexpr unsigned long sleep_timeout = 80000;    // time (ms) until turning off screen to save power
constexpr unsigned long leaving_timeout = 5000;   // time (ms) "thank you" scene is displayed
constexpr unsigned long cancelled_timeout = 5000; // time (ms) to stay in the cancelled state

// time (ms) to purge for, in setup menu
constexpr unsigned long purge_time = 5000;

// initial quantity of liquid in container (ml)
constexpr double syrup_quantity = 1890.0;
constexpr double soda_quantity = 2000.0;

// threshold when liquid is considered 'low'
constexpr double liquid_threshold = 0.1;

// amount (ml) to dispense, maps selection index to oz
constexpr std::array<double, 3> dispense_amount{
    2.0 * OZ_TO_ML,
    4.0 * OZ_TO_ML,
    8.0 * OZ_TO_ML
};

// ratio of soda in mixed drink, maps selection index
// i.e. 0.6 is 6 parts soda to 4 parts lotus
constexpr std::array<double, 3> soda_ratio{
    0.9,
    0.8,
    0.7
};

// https://www.directindustry.com/prod/kamoer-fluid-tech-shanghai-co-ltd/product-242598-2511427.html
// in ml/ms, will probably need to be tuned
constexpr double flow_rate = FLOW_RATE / (60 * 1000);

}; // namespace constants

template <uint8_t Mode, size_t N>
static void set_modes(const std::array<uint8_t, N>& pins) {
    for (size_t i = 0; i < N; i++) pinMode(pins[i], Mode);
}

template <size_t Start, size_t N>
static void set_motors(const std::array<uint8_t, N>& state) {
    for (size_t i = 0; i < N; i++) digitalWrite(constants::pins_motor[i + Start], state[i]);
}

template <size_t M>
inline void enable_motor() {
    lotus::set_motors<M, 1>({HIGH});
}

template <size_t M>
inline void disable_motor() {
    lotus::set_motors<M, 1>({LOW});
}

inline void disable_motors() {
    lotus::set_motors<0, 3>({LOW, LOW, LOW});
}

inline bool is_soda_low(double level) {
    return level / constants::soda_quantity < constants::liquid_threshold;
}

inline bool is_syrup_low(double level) {
    return level / constants::syrup_quantity < constants::liquid_threshold;
}

// helper classes

enum class Flavor : uint8_t {
    Pink,
    Pineapple,
    PassionFruit,
    ChocolateMint,
    Raspberry,
    Blueberry,
    PinkSplash,
    Count // append to find size
};

constexpr std::array flavor_text{
    "Pink",
    "Pineapple",
    "Passion Fruit",
    "Chocolate Mint",
    "Raspberry",
    "Blueberry",
    "Pink Splash"
};

inline std::string_view to_string(const Flavor& flavor) {
    return flavor_text[static_cast<size_t>(flavor)];
}

} // namespace lotus
