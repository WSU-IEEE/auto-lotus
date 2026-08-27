#pragma once

#include "scene.hpp"
#include "state.hpp"
#include "graphics.hpp"
#include "util.hpp"

// state machine stuff

enum class State {
    Home,
    SelectAmount,
    SelectRatio,
    Dispensing,
    Leaving,
    Fault,
    Count // appended so compiler can get the number of enums
};

using Machine = StateMachine<State>;

#define RESET_MOTORS setMotors<0, 4>({LOW, LOW, LOW, LOW});

class Home : public StateHandler<State, State::Home> {
public:
    void onEnter(Machine& machine) override {
        asleep = false;

        schedule(machine);
    }

    void onInput(Machine& machine, unsigned int input) override {
        if (asleep) {
            asleep = false;
            machine.display.setPowerSave(0);
            machine.scheduler.reset();
            schedule(machine);
        } else {
            machine.transition(State::SelectAmount);
        }
    }

    void onUpdate(Machine& machine) override {
        machine.display.clearBuffer();
        graphics::home_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }
private:
    volatile bool asleep = false;

    void onTimeout(Machine& machine) {
        asleep = true;
        machine.display.setPowerSave(1);
    }

    inline void schedule(Machine& machine) {
#ifndef DISABLE_POWER_SAVING
        machine.scheduler.schedule(constants::sleep_timeout, [this, &machine](){ this->onTimeout(machine); });
#endif
    }
};

class SelectAmount : public StateHandler<State, State::SelectAmount> {
public:
    void onEnter(Machine& machine) override {
        machine.scheduler.schedule(constants::timeout, [this, &machine](){ this->onTimeout(machine); });

        machine.display.clearBuffer();
        graphics::select_amount_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }

    void onInput(Machine& machine, unsigned int input) override {
        amount = constants::dispense_amount[input];
        Serial.println(amount);
        machine.transition(State::SelectRatio);
    }

    double amount = 0.0;
private:
    void onTimeout(Machine& machine) {
        machine.transition(State::Home);
    }
};

class SelectRatio : public StateHandler<State, State::SelectRatio> {
public:
    void onEnter(Machine& machine) override {
        machine.scheduler.schedule(constants::timeout, [this, &machine](){ this->onTimeout(machine); });

        machine.display.clearBuffer();
        graphics::select_ratio_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }

    void onInput(Machine& machine, unsigned int input) override {
        ratio = constants::soda_ratio[input];
        Serial.println(machine.getHandler<SelectAmount>().amount);
        machine.transition(State::Dispensing);
    }

    double ratio = 0.0;
private:
    void onTimeout(Machine& machine) {
        machine.transition(State::Home);
    }
};

class Dispensing : public StateHandler<State, State::Dispensing> { // TODO add a "menu" to confirm halting dispensing
public:
    void onEnter(Machine& machine) override {
        dispensed = false;

        setMotors<0, 4>({LOW, HIGH, LOW, HIGH});

#ifdef DEBUG_DISPENSE_TIME
        const unsigned long total_time = DEBUG_DISPENSE_TIME;
#else
        const unsigned long total_time = 1.0 / constants::flow_rate * machine.getHandler<SelectAmount>().amount;
#endif
        const unsigned long soda_time = total_time * machine.getHandler<SelectRatio>().ratio;
        const unsigned long lotus_time = total_time - soda_time;

        Serial.println(constants::flow_rate);
        Serial.println(total_time);
        Serial.println(soda_time);
        Serial.println(lotus_time);

        machine.scheduler.schedule(soda_time, [this, &machine](){ this->onDispensed<0, 2>(machine); });
        machine.scheduler.schedule(lotus_time, [this, &machine](){ this->onDispensed<2, 2>(machine); });

        const auto now = millis();

        auto& dispense = graphics::dispensing_scene.get<custom::Dispense>();
        dispense.start = now;
        dispense.duration = max(soda_time, lotus_time);
        Serial.printf("duration: %lu", dispense.duration);
    }

    bool onExit(Machine& machine) override {
        RESET_MOTORS
        return true;
    }

    void onInput(Machine& machine, unsigned int input) override {
        machine.transition(State::Fault);
    }

    void onUpdate(Machine& machine) override {
        machine.display.clearBuffer();
        graphics::dispensing_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }
private:
    volatile bool dispensed = false; // use atomic?

    template <size_t Start, size_t Count>
    void onDispensed(Machine& machine) {
        setMotors<Start, Count>({LOW, LOW});
        if (dispensed) machine.transition(State::Leaving);
        else dispensed = true;
    }
};

class Leaving : public StateHandler<State, State::Leaving> {
public:
    void onEnter(Machine& machine) override {
        machine.scheduler.schedule(constants::leaving_timeout, [this, &machine](){ this->onTimeout(machine); });

        machine.display.clearBuffer();
        graphics::thank_you_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }

    void onInput(Machine& machine, unsigned int input) override {
        machine.transition(State::Home);
    }
private:
    void onTimeout(Machine& machine) {
        machine.transition(State::Home);
    }
};

class Fault : public StateHandler<State, State::Fault> {
public:
    void onEnter(Machine& machine) override {
        machine.display.clearBuffer();
        graphics::fault_scene.draw(machine.display, millis());
        machine.display.sendBuffer();
    }

    void onInput(Machine& machine, unsigned int input) override {
        Serial.println("cannot exit fault state!");
    }

    bool onExit(Machine& machine) override {
        return false;
    }
};
