#pragma once

#include "scheduler.hpp"
#include "input.hpp"
#include "util.hpp"

template <typename E, typename Context>
class StateMachine;

template <typename E, typename Context>
class IStateHandler {
public:
    virtual void on_enter(StateMachine<E, Context>& machine, unsigned long now) = 0;
    virtual bool on_exit(StateMachine<E, Context>& machine, unsigned long now) {
        return true;
    };
    virtual void on_input(StateMachine<E, Context>& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) = 0;
    virtual void on_update(StateMachine<E, Context>& machine, unsigned long now) { };
};

template <typename E, E State, typename Context>
class StateHandler : public IStateHandler<E, Context> {
public:
    static constexpr E state = State;
};

template <typename E, typename Context>
class StateMachine {
public:
    Context context;
    Scheduler scheduler;
protected:
    IStateHandler<E, Context>* m_handlers[static_cast<size_t>(E::Count)]; // you must append the enum Count to the end of the list in order to get the length
    E m_current;
public:
    StateMachine(Context& context) : context(context) { };

    template <typename H>
    H& get_handler() {
        static H handler;
        return handler;
    }

    template <typename H>
    void add() {
        constexpr E state = H::state;
        m_handlers[static_cast<size_t>(state)] = &get_handler<H>();
    }

    void transition(E state, unsigned long now) {
        auto cancelled = !m_handlers[static_cast<size_t>(m_current)]->on_exit(*this, now);
        if (cancelled)
            return;
        m_current = state;
        scheduler.reset(now);
        m_handlers[static_cast<size_t>(state)]->on_enter(*this, now);
    }

    void input(const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) {
        m_handlers[static_cast<size_t>(m_current)]->on_input(*this, pressed, now);
    }

    void update(unsigned long now) {
        m_handlers[static_cast<size_t>(m_current)]->on_update(*this, now);
        scheduler.process(now);
    }
};
