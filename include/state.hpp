#pragma once

#include "scheduler.hpp"

template <typename E>
class StateMachine;

template <typename E>
class IStateHandler {
public:
    virtual void on_enter(StateMachine<E>& machine, unsigned long now) = 0;
    virtual bool on_exit(StateMachine<E>& machine, unsigned long now) {
        return true;
    };
    virtual void on_input(StateMachine<E>& machine, unsigned int input, unsigned long now) = 0;
    virtual void on_update(StateMachine<E>& machine, unsigned long now) { };
};

template <typename E, E State>
class StateHandler : public IStateHandler<E> {
public:
    static constexpr E state = State;
};

template <typename E>
class StateMachine {
public:
    StateMachine(U8G2& display) : display(display) { };

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

    void input(unsigned pressed, unsigned long now) {
        m_handlers[static_cast<size_t>(m_current)]->on_input(*this, pressed, now);
    }

    void update(unsigned long now) {
        m_handlers[static_cast<size_t>(m_current)]->on_update(*this, now);
        scheduler.process(now);
    }

    U8G2& display;
    Scheduler scheduler;
protected:
    IStateHandler<E>* m_handlers[static_cast<size_t>(E::Count)]; // you must append the enum Count to the end of the list in order to get the length
    E m_current;
};
