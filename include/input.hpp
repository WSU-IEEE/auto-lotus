#pragma once

#include "util.hpp"
#include "scheduler.hpp"

template <size_t N>
class ButtonInput {
public:
    enum class Type : uint8_t {
        Neutral, Pressed, Stale
    };
private:
    const std::array<uint8_t, N>& m_pins;
    std::array<Type, N> m_pressed{};
public:
    constexpr ButtonInput(const std::array<uint8_t, N>& pins)
    : m_pins(pins)
    { }

    void setup() {
        set_modes<INPUT>(m_pins);
    }

    bool poll() {
        int read[N];
        for (size_t i = 0; i < N; ++i)
            read[i] = digitalRead(m_pins[i]);

        bool event = false;

        for (size_t i = 0; i < N; i++) {
            if (!read[i]) { // read at LOW means pressed
                if(m_pressed[i] == Type::Pressed)
                    m_pressed[i] = Type::Stale;
                else if (!static_cast<uint8_t>(m_pressed[i])) {
                    m_pressed[i] = Type::Pressed;
                    event = true;
                }
            } else if (static_cast<uint8_t>(m_pressed[i]))
                m_pressed[i] = Type::Neutral;
        }

        return event;
    }

    int button() {
        for (size_t i = 0; i < N; i++)
            if (m_pressed[i] == Type::Pressed)
                return i;
        return -1;
    }

    const std::array<Type, N>& get() const {
        return m_pressed;
    }
};

template <size_t Buttons, size_t Events>
class SimulatedInput { // TODO this is not working!
private:
    Scheduler m_scheduler;
    std::array<uint8_t, Buttons> m_pressed{};
    bool m_stale = false;
public:
    struct Event {
        unsigned int time;
        std::array<uint8_t, Buttons> buttons;
    };
public:
    SimulatedInput(const std::array<Event, Events>& inputs) {
        for (size_t i = 0; i < Events; i++)
            m_scheduler.schedule(
                inputs[i].time,
                [this, inputs, i]() {
                    this->press(inputs[i].button);
                }
            );
    }

    void setup() { }

    bool poll(std::array<uint8_t, Buttons>& buttons) {
        m_scheduler.process();
        auto button = m_pressed;
        if (m_stale)
            return false;
        m_stale = true;
        buttons = m_pressed;
        return true;
    }

    void press(const std::array<uint8_t, Buttons>& buttons) {
        m_pressed = buttons;
        m_stale = false;
    }
};
