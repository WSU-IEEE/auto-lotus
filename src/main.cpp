#include <Wire.h>

#include "auto.hpp"
#include "graphics.hpp"

#define RESET_MOTORS setMotors({LOW, LOW, LOW, LOW});

/*
 * https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#constructor-name
 * https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#sh1106-128x64_noname-1
 * using full frame buffer (F) and Arduino Wire library for communication.
 *
 * U8X8_PIN_HOME used as no reset connected to display.
 * U8G2_R0 to indicate on rotation.
*/
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

using Transition = void (*)();

void toSleep() {
    u8g2.drawXBMP(0, 0, Graphics::sleep.width, Graphics::sleep.height, Graphics::sleep.bitmap);
    u8g2.sendBuffer();
}

void toSelectAmount() {
    u8g2.drawXBMP(0, 0, Graphics::select_amount.width, Graphics::select_amount.height, Graphics::select_amount.bitmap);
    u8g2.sendBuffer();
}

void toSelectRatio() {
    u8g2.drawXBMP(0, 0, Graphics::select_ratio.width, Graphics::select_ratio.height, Graphics::select_ratio.bitmap);
    u8g2.sendBuffer();
}

void toDispensing() {
    setMotors({LOW, HIGH, LOW, HIGH});
    u8g2.drawXBMP(0, 0, Graphics::dispensing.width, Graphics::dispensing.height, Graphics::dispensing.bitmap);
    u8g2.sendBuffer();
}

constexpr Transition transitions[] = {
    toSleep,
    toSelectAmount,
    toSelectRatio,
    toDispensing
};

State state = State::Sleep;
unsigned long time = 0; // time current state was reached

void transition(State next) {
    state = next, time = millis();
    transitions[static_cast<size_t>(next)]();
}

int last[3] = { 0 }; // raw button state last time they were read
unsigned long debounce[3] = { 0 }; // time when button was first pressed
bool stable[3] = { false }; // debounced button state

double amount = 0.0; // amount of liquid to dispense
double ratio = 0.0; // ratio of soda in mix

void setup() {
    Serial.begin(115200); // Baud rate
    Wire.begin(pin_I2C_sda, pin_I2C_scl);
    u8g2.begin();

    // Buttons with pull-down resistors makes sure they read LOW when not pressed and HIGH when pressed
    setModes<3, INPUT_PULLDOWN>(pins_btn);
    // Outputs to control the L298N motor driver (assuming IN1, IN2 for motor 1 and IN3, IN4 for motor 2)
    setModes<4, OUTPUT>(pins_motor);

    transition(State::Sleep);
}

void loop() {
    const int read[3] = {
        digitalRead(pins_btn[0]), // left
        digitalRead(pins_btn[1]), // middle
        digitalRead(pins_btn[2])  // right
    };

    const unsigned long now = millis();

    int pressed = -1;

    for (size_t i = 0; i < 3; i++) {
        if (read[i] != last[i]) debounce[i] = now, last[i] = read[i];
        if ((now - debounce[i]) >= debounce_delay) {
            if (read[i] && !stable[i]) stable[i] = true, pressed = i;
            else if (!read[i] && stable[i]) stable[i] = false;
        }
    }

    switch (state) { // add handler for accepting user input instead of switch statement
    case State::Sleep:
        if (pressed != -1) transition(State::SelectAmount);
        else if (now - time >= sleep_timeout) u8g2.setPowerSave(1); // queue sleep event instead so it is only fired once.
        break;
    case State::SelectAmount:
        if (pressed != -1) {
            amount = dispense_amount[pressed];
            transition(State::SelectRatio);
        } else if (now - time >= timeout) transition(State::Sleep); // also event
        break;
    case State::SelectRatio:
        if (pressed != -1) {
            ratio = soda_ratio[pressed];
            transition(State::Dispensing);
        } else if (now - time >= timeout) transition(State::Sleep); // another event
        break;
    case State::Dispensing:
        // TODO make this actually care about ratios by using two events! one turns of motor one, the other turns off motor two.
        if (pressed != -1) {
            RESET_MOTORS;
            transition(State::Sleep);
        } else if (now - time >= 1.0 / flow_rate * amount) { // should be event
            RESET_MOTORS;
            transition(State::Sleep);
            // TODO show a little thank you message?
        };
        break;
    }

    // handle events here

    delay(20);
}
