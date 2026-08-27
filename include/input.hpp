#pragma once

#include "util.hpp"
#include "scheduler.hpp"

class InputSource {
public:
    virtual void setup() = 0;
    virtual int poll() = 0;
};

template <size_t N>
class ButtonSource : public InputSource {
public:
    constexpr ButtonSource(const uint8_t (&pins)[N]) : pins(pins) { }

    void setup() override {
        setModes<INPUT_PULLDOWN>(pins);
    }

    int poll() override {
        int read[N];
        for (size_t i = 0; i < N; ++i) read[i] = digitalRead(constants::pins_btn[i]);

        const auto now = millis();
        int pressed = -1;

        for (size_t i = 0; i < N; i++) {
            if (read[i] != last[i]) debounce[i] = now, last[i] = read[i];
            if ((now - debounce[i]) >= constants::debounce_delay) {
                if (read[i] && !stable[i]) stable[i] = true, pressed = i;
                else if (!read[i] && stable[i]) stable[i] = false;
            }
        }

        return pressed;
    }
private:
    int last[N] = { 0 }; // raw button state last time they were read
    unsigned long debounce[N] = { 0 }; // time when button was first pressed
    bool stable[N] = { false }; // debounced button state

    const uint8_t (&pins)[N];
};

class SimulatedSource : public InputSource {
public:
    struct InputEvent {
        unsigned int time;
        unsigned int button;
    };

    template <size_t N>
    SimulatedSource(const SimulatedSource::InputEvent (&inputs)[N]) {
        for (size_t i = 0; i < N; i++) scheduler.schedule(inputs[i].time, [this, inputs, i](){ this->press(inputs[i].button); });
    }

    void setup() override { }

    int poll() override {
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
