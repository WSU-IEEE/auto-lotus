#pragma once

//#include "graphics.hpp"
#include <Arduino.h>
#include "graphics.hpp"
#include "scene.hpp"
#include "scheduler.hpp"
#include "auto.hpp"

#define RESET_MOTORS setMotors<0, 4>({LOW, LOW, LOW, LOW});

enum class State {
    Home,
    SelectAmount,
    SelectRatio,
    Dispensing
};

class StateMachine;

class IStateHandler {
public:
    virtual void onEnter(StateMachine& machine) = 0;
    virtual void onExit(StateMachine& machine) { };
    virtual void onInput(StateMachine& machine, unsigned int input) = 0;
    virtual void onUpdate(StateMachine& machine) { };
};

template <State S>
class StateHandler : public IStateHandler {
public:
    static constexpr State state = S;
};

class StateMachine {
public:
    StateMachine(U8G2& display) : display(display) { };

    template <typename Handler>
    Handler& getHandler() {
        static Handler handler;
        return handler;
    }

    template <typename Handler>
    void add() {
        constexpr State state = Handler::state;
        handlers[static_cast<size_t>(state)] = &getHandler<Handler>();
    }

    void transition(State state) {
        handlers[static_cast<size_t>(current)]->onExit(*this);
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
    IStateHandler* handlers[4]{};
    State current = State::Home;
    unsigned long time = 0;
};

class Home : public StateHandler<State::Home> {
public:
    void onEnter(StateMachine& machine) override {
        Serial.println("to sleep");
        asleep = false;

        schedule(machine);
        machine.display.clearBuffer();
        Graphics::home_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }

    void onInput(StateMachine& machine, unsigned int input) override {
        if (asleep) {
            asleep = false;
            machine.display.setPowerSave(0);
            machine.scheduler.reset();
            schedule(machine);
        } else {
            machine.transition(State::SelectAmount);
        }
    }

    void onUpdate(StateMachine& machine) override {
        machine.display.clearBuffer();
        Graphics::home_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }
private:
    bool asleep = false;

    void onTimeout(StateMachine& machine) {
        asleep = true;
        machine.display.setPowerSave(1);
    }

    inline void schedule(StateMachine& machine) {
        machine.scheduler.schedule(sleep_timeout, [this, &machine](){ this->onTimeout(machine); });
    }
};

class SelectAmount : public StateHandler<State::SelectAmount> {
public:
    void onEnter(StateMachine& machine) override {
        Serial.println("to amount");
        machine.scheduler.schedule(timeout, [this, &machine](){ this->onTimeout(machine); });

        machine.display.clearBuffer();
        //Graphics::select_amount_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }

    void onInput(StateMachine& machine, unsigned int input) override {
        amount = dispense_amount[input];
        Serial.println(amount);
        machine.transition(State::SelectRatio);
    }

    double amount = 0.0;
private:
    void onTimeout(StateMachine& machine) { // TODO resolve binding machine
        machine.transition(State::Home);
    }
};

class SelectRatio : public StateHandler<State::SelectRatio> {
public:
    void onEnter(StateMachine& machine) override {
        Serial.println("to ratio");
        machine.scheduler.schedule(timeout, [this, &machine](){ this->onTimeout(machine); });

        machine.display.clearBuffer();
        //Graphics::select_ratio_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }

    void onInput(StateMachine& machine, unsigned int input) override {
        ratio = soda_ratio[input];
        Serial.println(machine.getHandler<SelectAmount>().amount);
        machine.transition(State::Dispensing);
    }

    double ratio = 0.0;
private:
    void onTimeout(StateMachine& machine) { // TODO resolve binding machine
        machine.transition(State::Home);
    }
};

class Dispensing : public StateHandler<State::Dispensing> {
public:
    void onEnter(StateMachine& machine) override {
        Serial.println("to dispense");
        dispensed = false;

        setMotors<0, 4>({LOW, HIGH, LOW, HIGH});

        const unsigned long total_time = 1.0 / flow_rate * machine.getHandler<SelectAmount>().amount;
        const unsigned long soda_time = total_time * machine.getHandler<SelectRatio>().ratio;
        const unsigned long lotus_time = total_time - soda_time;

        Serial.println(flow_rate);
        Serial.println(total_time);
        Serial.println(soda_time);
        Serial.println(lotus_time);

        machine.scheduler.schedule(soda_time, [this, &machine](){ this->onDispensed<0, 2>(machine); });
        machine.scheduler.schedule(lotus_time, [this, &machine](){ this->onDispensed<2, 2>(machine); });

        machine.display.clearBuffer();
        //Graphics::dispensing_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }

    void onExit(StateMachine& machine) override {
        RESET_MOTORS
    }

    void onInput(StateMachine& machine, unsigned int input) override {
        machine.display.clearBuffer();
        //Graphics::already_dispensing_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }
private:
    bool dispensed = false;

    template <size_t Start, size_t Count>
    void onDispensed(StateMachine& machine) {
        setMotors<Start, Count>({LOW, LOW});
        if (dispensed) machine.transition(State::Home);
        else dispensed = true;
    }
};
