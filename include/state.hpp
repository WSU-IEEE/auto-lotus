#pragma once

#include "scheduler.hpp"

template <typename TEnum>
class StateMachine;

template <typename TEnum>
class IStateHandler {
public:
    virtual void onEnter(StateMachine<TEnum>& machine) = 0;
    virtual bool onExit(StateMachine<TEnum>& machine) {
        return true;
    };
    virtual void onInput(StateMachine<TEnum>& machine, unsigned int input) = 0;
    virtual void onUpdate(StateMachine<TEnum>& machine) { };
};

template <typename TEnum, TEnum TState>
class StateHandler : public IStateHandler<TEnum> {
public:
    static constexpr TEnum state = TState;
};

template <typename TEnum>
class StateMachine {
public:
    StateMachine(U8G2& display) : display(display) { };

    template <typename THandler>
    THandler& getHandler() {
        static THandler handler;
        return handler;
    }

    template <typename THandler>
    void add() {
        constexpr TEnum state = THandler::state;
        handlers[static_cast<size_t>(state)] = &getHandler<THandler>();
    }

    void transition(TEnum state) {
        auto cancelled = !handlers[static_cast<size_t>(current)]->onExit(*this);
        if (cancelled) return;
        time = millis();
        current = state;
        scheduler.reset();
        handlers[static_cast<size_t>(state)]->onEnter(*this);
        Serial.println("transitioned");
    }

    void input(unsigned pressed) {
        handlers[static_cast<size_t>(current)]->onInput(*this, pressed);
    }

    void update() {
        handlers[static_cast<size_t>(current)]->onUpdate(*this);
        scheduler.process();
    }

    U8G2& display;
    Scheduler scheduler;
protected:
    IStateHandler<TEnum>* handlers[static_cast<size_t>(TEnum::Count)]; // you must append the enum Count to the end of the list in order to get the length
    TEnum current;
    unsigned long time = 0;
};
