#include <Wire.h>

#include "util.hpp"
#include "lotus.hpp"
#include "input.hpp"

// #define SIMULATE_INPUT

/*
 * https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#constructor-name
 * https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#sh1106-128x64_noname-1
 * using full frame buffer (F) and Arduino Wire library for communication.
 *
 * U8X8_PIN_HOME used as no reset connected to display.
 * U8G2_R0 to indicate on rotation.
*/
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

Machine machine(u8g2);

#ifdef SIMULATE_INPUT
constexpr SimulatedSource::InputEvent simulated_buttons[] = {
    {5000, 0},
    {10000, 0},
    {15000, 0},
    //{16000, 0}
};

SimulatedSource input(simulated_buttons);
#else
ButtonSource input(constants::pins_btn);
#endif

void setup() {
    Serial.begin(115200); // baud rate
    Wire.begin(constants::pin_I2C_sda, constants::pin_I2C_scl);
    u8g2.begin();
    u8g2.setFontPosTop();
    u8g2.setFontMode(1);

    // buttons with pull-down resistors makes sure they read LOW when not pressed and HIGH when pressed
    input.setup();
    // outputs to control the L298N motor driver (assuming IN1, IN2 for motor 1 and IN3, IN4 for motor 2)
    setModes<OUTPUT>(constants::pins_motor);

    machine.add<Home>();
    machine.add<SelectAmount>();
    machine.add<SelectRatio>();
    machine.add<Dispensing>();
    machine.add<Leaving>();
    machine.add<Fault>();
    machine.transition(State::Home);

    Serial.println("setup complete!");
}

void loop() {
    int pressed = input.poll();
    if (pressed != -1) machine.input(pressed);

    machine.update();

    delay(20);
}
