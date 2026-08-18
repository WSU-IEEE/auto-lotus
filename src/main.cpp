#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "graphics.hpp"


// button debounce time
const unsigned long debounceDelay = 200;

// button pins
const uint8_t btn[3] = {
    19, // 8oz
    18, // 16oz
    17  // 32oz
};

// motor controller pins
const uint8_t motor[4] = {
    16, // IN1
    15, // IN2
    14, // IN3
    12  // IN4
};

/*
    https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#constructor-name
    https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#sh1106-128x64_noname-1
    using full frame buffer (F) and Arduino Wire library for communication.

    U8X8_PIN_HOME used as no reset connected to display.
    U8G2_R0 to indicate on rotation.
*/
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

int last[3] = { 0 }; // raw button state last time they were read
unsigned long debounce[3] = { 0 }; // time when button was first pressed
bool stable[3] = { false }; // debounced button state
 
// Screen variable initialization
int currentScreen = 0; // variable to track current screen
const int totalScreens = 5; // variable for total number of screens (home + 4 dispense states)

void setup() {
    Serial.begin(115200); // Baud rate

    Wire.begin(21, 22); // I2C pins for ESP32 (SDA, SCL)

    u8g2.begin(); // Initialize the display

    // Buttons with pull-down resistors makes sure they read LOW when not pressed and HIGH when pressed 
    pinMode(btn[0], INPUT_PULLDOWN); // 8oz
    pinMode(btn[1], INPUT_PULLDOWN); // 16oz
    pinMode(btn[2], INPUT_PULLDOWN); // 32oz

    // Outputs to control the L298N motor driver (assuming IN1, IN2 for motor 1 and IN3, IN4 for motor 2)
    pinMode(motor[0], OUTPUT); // Motor 1 Control IN1
    pinMode(motor[1], OUTPUT); // Motor 1 Control IN2 
    pinMode(motor[2], OUTPUT); // Motor 2 Control IN3
    pinMode(motor[3], OUTPUT); // Motor 2 Control IN4
}

enum State {
    HOME, // Default screen
    DISPENSE_8, // 8 oz dispense
    DISPENSE_16, // 16 oz dispense
    DISPENSE_32, // 32 oz dispense
    ALREADY // Already dispensing
};

State state = HOME; // initialize state to HOME
unsigned long stateStartTime = 0; // time current state was reached
const unsigned long dispenseTime = 10000; // dispense time

void loop() {
    const int read[3] = {
        digitalRead(btn[0]), // left
        digitalRead(btn[1]), // middle
        digitalRead(btn[2])  // right
    };

    const unsigned long now = millis();

    bool pressed[3] = { false };
    bool any = false;

    for (size_t i = 0; i < 3; i++) {
        if (read[i] != last[i]) debounce[i] = now, last[i] = read[i];
        if ((now - debounce[i]) >= debounceDelay && read[i] && !stable[i]) stable[i] = pressed[i] = any = true;
        if (!read[i] && stable[i]) stable[i] = false;
    }

    switch (state) {
    case HOME:
        if (pressed[0]) state = DISPENSE_8;
        else if (pressed[1]) state = DISPENSE_16;
        else if (pressed[2]) state = DISPENSE_32;
        if (any) stateStartTime = now;
        break;
    case ALREADY:
        if (now - stateStartTime >= dispenseTime)
            state = HOME;
        break;
    default:
        if (now - stateStartTime >= dispenseTime) state = HOME;
        else if (any) state = ALREADY;
    }

    bool dispensing = (state == DISPENSE_8 || state == DISPENSE_16 || state == DISPENSE_32);

    if (dispensing) { // dispense
        digitalWrite(motor[0], HIGH);
        digitalWrite(motor[1], LOW);
        digitalWrite(motor[2], HIGH);
        digitalWrite(motor[3], LOW);
    } else { // turn motors off
        digitalWrite(motor[0], LOW);
        digitalWrite(motor[1], LOW);
        digitalWrite(motor[2], LOW);
        digitalWrite(motor[3], LOW);
    }

    u8g2.clearBuffer();

    switch (state) {
    case HOME:
        u8g2.drawXBMP(0, 0, 128, 64, Graphics::Home);
        break;
    case DISPENSE_8:
        u8g2.drawXBMP(0, 0, 128, 64, Graphics::Dispense8);
        break;
    case DISPENSE_16:
        u8g2.drawXBMP(0, 0, 128, 64, Graphics::Dispense16);
        break;
    case DISPENSE_32:
        u8g2.drawXBMP(0, 0, 128, 64, Graphics::Dispense32);
        break;
    case ALREADY:
        u8g2.drawXBMP(0, 0, 128, 64, Graphics::AlreadyDispensing);
        break;
    }

    u8g2.sendBuffer();

    delay(20);
}