#pragma once

#include "util.hpp"
#include "scheduler.hpp"

namespace input {

enum class Type : uint8_t {
    Neutral, Pressed, Stale
};

template <size_t N>
int button(const std::array<Type, N> pressed) {
    for (size_t i = 0; i < N; i++)
        if (pressed[i] == Type::Pressed)
            return i;
    return -1;
}

template <size_t N>
class ButtonInput {
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

    bool poll(unsigned long now) {
        int read[N];
        for (size_t i = 0; i < N; ++i)
            read[i] = digitalRead(m_pins[i]);

        bool event = false;

        for (size_t i = 0; i < N; i++) {
            if (read[i]) {
                if(m_pressed[i] == Type::Pressed)
                    m_pressed[i] = Type::Stale;
                else if (m_pressed[i] == Type::Neutral) {
                    m_pressed[i] = Type::Pressed;
                    event = true;
                }
            } else if (m_pressed[i] != Type::Neutral)
                m_pressed[i] = Type::Neutral;
        }

        return event;
    }

    int button() {
        return button(m_pressed);
    }

    const std::array<Type, N>& get() const {
        return m_pressed;
    }
};

template<size_t N>
struct Event {
    unsigned int time;
    std::array<Type, N> buttons;
};

template <size_t N>
class SimulatedInput {
private:
    Scheduler m_scheduler;
    std::array<Type, N> m_pressed{};
    bool m_stale = false;
public:
    using Event = struct input::Event<N>;
public:
    template <size_t E>
    explicit SimulatedInput(const std::array<Event, E>& inputs) {
        for (size_t i = 0; i < E; i++)
            m_scheduler.schedule(
                inputs[i].time,
                [this, inputs, i]() {
                    this->press(inputs[i].buttons);
                }
            );
    }

    void setup() { }

    bool poll(unsigned long now) {
        m_scheduler.process(now);
        if (m_stale)
            return false;
        m_stale = true;
        return true;
    }

    void press(const std::array<Type, N>& buttons) {
        m_pressed = buttons;
        m_stale = false;
    }

    int button() const {
        return button(m_pressed);
    }

    const std::array<Type, N>& get() const {
        return m_pressed;
    }
};

template <size_t N>
constexpr std::array<Type, N> button_pressed(size_t index) {
    std::array<Type, N> buttons{};

    if (index < N)
        buttons[index] = Type::Pressed;

    return buttons;
}

template <size_t N, size_t E>
SimulatedInput(const std::array<Event<N>, E>&) -> SimulatedInput<N>;

} // namespace input
