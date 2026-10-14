#include <Wire.h>

#include "toast.hpp"
#include "util.hpp"
#include "lotus.hpp"
#include "input.hpp"

#define SIMULATE_INPUT

/*
 * https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#constructor-name
 * https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#sh1106-128x64_noname-1
 * using full frame buffer (F) and Arduino Wire library for communication.
 *
 * U8X8_PIN_HOME used as no reset connected to display.
 * U8G2_R0 to indicate on rotation.
*/
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
NotificationManager notifications(u8g2);

Context context{u8g2, notifications};

Machine machine(context);

#ifdef SIMULATE_INPUT
constexpr std::array simulated_buttons{
    input::Event<4>{10000, input::button_pressed<4>(0)},
    input::Event<4>{15000, input::button_pressed<4>(3)},
    //input::Event<4>{15000, {input::Type::Neutral, input::Type::Neutral, input::Type::Pressed, input::Type::Pressed}},
};

input::SimulatedInput input_provider(simulated_buttons);
#else
input::ButtonInput input_provider(constants::pins_btn);
#endif

void setup() {
    Serial.begin(115200); // baud rate
    Wire.begin(constants::pin_I2C_sda, constants::pin_I2C_scl);
    u8g2.begin();
    u8g2.setFontPosTop();
    u8g2.setFontMode(1);

    // buttons with pull-down resistors makes sure they read LOW when not pressed and HIGH when pressed
    input_provider.setup();
    // outputs to control the L298N motor driver (assuming IN1, IN2 for motor 1 and IN3, IN4 for motor 2)
    set_modes<OUTPUT>(constants::pins_motor);

    machine.add<Home>();
    machine.add<SelectAmount>();
    machine.add<SelectRatio>();
    machine.add<Dispensing>();
    machine.add<Leaving>();
    machine.add<Cancelled>();

    delay(1000);

    machine.transition(State::Home, millis());

    Serial.println("setup complete!");
}

void loop() {
    const unsigned long now = millis();

    const auto focus = notifications.process(now);

    if (input_provider.poll(now)) {
        if (focus)
            notifications.close();
        else
            machine.input(input_provider.get(), now);
    }

    u8g2.clearBuffer();

    machine.update(now);
    notifications.process(now);

    u8g2.sendBuffer();

    delay(20);
}
