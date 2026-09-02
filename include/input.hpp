#pragma once

#include "esp32-hal-gpio.h"
#include "util.hpp"
#include "scheduler.hpp"

template <size_t N>
class ButtonSource {
public:
    constexpr ButtonSource(const uint8_t (&pins)[N]) : pins(pins) { }

    void setup() {
        setModes<INPUT>(pins);
    }

    int poll() {
        int read[N];
        for (size_t i = 0; i < N; ++i)
            read[i] = digitalRead(pins[i]);

        int pressed = -1;

        // TODO add raw input option
        for (size_t i = 0; i < N; i++)
            if (!read[i])
                pressed = i;

        return pressed;
    }
private:
    const uint8_t (&pins)[N];
};

class SimulatedSource {
public:
    struct InputEvent {
        unsigned int time;
        unsigned int button;
    };

    template <size_t N>
    SimulatedSource(const SimulatedSource::InputEvent (&inputs)[N]) {
        for (size_t i = 0; i < N; i++) scheduler.schedule(inputs[i].time, [this, inputs, i](){ this->press(inputs[i].button); });
    }

    void setup() { }

    int poll() {
        scheduler.process();
        auto button = pressed;
        if (pressed != -1) pressed = -1;
        return button;
    }
private:
    Scheduler scheduler;
    int pressed = -1;

    void press(unsigned int button) {
        pressed = button;
    }
};
