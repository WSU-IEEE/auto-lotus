#include <Wire.h>
#include <esp32-hal-adc.h>

#include "custom.hpp"
#include "toast.hpp"
#include "util.hpp"
#include "lotus.hpp"
#include "input.hpp"
#include "storage.hpp"
#include "logging.hpp"
#include "ansi.hpp"

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
notification::NotificationManager notifications(u8g2);
Storage storage;

lotus::Context context{u8g2, notifications, storage};

lotus::Machine machine(context);

logging::Logger logger{"main"};

#ifdef SIMULATE_INPUT
constexpr std::array simulated_buttons{
    input::Event<4>{10000, input::button_pressed<4>(0)},
    input::Event<4>{15000, input::button_pressed<4>(3)},
    //input::Event<4>{15000, {input::Type::Neutral, input::Type::Neutral, input::Type::Pressed, input::Type::Pressed}},
};

input::SimulatedInput input_provider(simulated_buttons);
#else
// buttons are active low (default)
input::ButtonInput input_provider(lotus::constants::pins_btn);
#endif

void setup() {
    logging::log_level = logging::Level::Debug;
    Serial.begin(115200); // baud rate
    Wire.begin(lotus::constants::pin_I2C_sda, lotus::constants::pin_I2C_scl);
    u8g2.begin();
    u8g2.setFontPosTop();
    u8g2.setFontMode(1);

    storage.setup();

    analogReadResolution(12);

    analogSetPinAttenuation(MOTOR_OUT_1, ADC_11db);
    analogSetPinAttenuation(MOTOR_OUT_2, ADC_11db);

    input_provider.setup();

    lotus::set_modes<OUTPUT>(lotus::constants::pins_motor);

    machine.add<lotus::Setup>();
    machine.add<lotus::Home>();
    machine.add<lotus::SelectAmount>();
    machine.add<lotus::SelectRatio>();
    machine.add<lotus::Dispensing>();
    machine.add<lotus::Leaving>();
    machine.add<lotus::Cancelled>();

    delay(1000);

    machine.transition(lotus::State::Setup, millis());
    //machine.transition(lotus::State::Home, millis());

    LOG_INFO(logger, FG_CYAN "=========================================================" ANSI_RESET);
    LOG_INFO(logger, FG_CYAN "    ___   __  ____________  __    ____  ________  _______" ANSI_RESET);
    LOG_INFO(logger, FG_CYAN "   /   | / / / /_  __/ __ \\/ /   / __ \\/_  __/ / / / ___/" ANSI_RESET);
    LOG_INFO(logger, FG_CYAN "  / /| |/ / / / / / / / / / /   / / / / / / / / / /\\__ \\" ANSI_RESET);
    LOG_INFO(logger, FG_CYAN " / ___ / /_/ / / / / /_/ / /___/ /_/ / / / / /_/ /___/ /" ANSI_RESET);
    LOG_INFO(logger, FG_CYAN "/_/  |_\\____/ /_/  \\____/_____/\\____/ /_/  \\____//____/" ANSI_RESET);
    LOG_INFO(logger, FG_RED "WSU " FG_BLUE "IEEE" FG_WHITE " 2026" ANSI_RESET);
    LOG_INFO(logger, FG_CYAN "=========================================================" ANSI_RESET);
}

void loop() {
    const unsigned long now = millis();

    const auto focus = notifications.process(now);

    if (input_provider.poll(now)) {
        const auto pressed = input_provider.get();
        if (pressed[0] != input::Type::Neutral && pressed[1] != input::Type::Neutral && pressed[2] != input::Type::Neutral)
            machine.transition(lotus::State::Setup, now);
        else {
            if (focus)
                notifications.close();
            else
                machine.input(pressed, now);
        }
    }

    u8g2.clearBuffer();

    machine.update(now);
    notifications.process(now);

    u8g2.sendBuffer();

    delay(20);
}
