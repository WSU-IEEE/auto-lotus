#pragma once

#include "input.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "graphics.hpp"
#include "util.hpp"
#include "toast.hpp"

// state machine stuff

enum class State {
    Home,
    SelectAmount,
    SelectRatio,
    Dispensing,
    Leaving,
    Cancelled,
    Count // appended so compiler can get the number of enums
};

struct Context {
    U8G2& display;
    NotificationManager& notifications;
};

using Machine = StateMachine<State, Context>;

#define RESET_MOTORS set_motors<0, 4>({LOW, LOW, LOW, LOW});

class Home : public StateHandler<State, State::Home, Context> {
public:
    void on_enter(Machine& machine, unsigned long now) override {
        machine.context.notifications.send(graphics::syrup_low);

        asleep = false;

        schedule(machine);
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        if (asleep) {
            asleep = false;
            machine.context.display.setPowerSave(0);
            machine.scheduler.reset(now);
            schedule(machine);
        } else {
            if (input::button(pressed) < 3)
                machine.transition(State::SelectAmount, now);
        }
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::home_scene.draw(machine.context.display, now);
    }
private:
    volatile bool asleep = false;

    void on_timeout(Machine& machine) {
        asleep = true;
        machine.context.display.setPowerSave(1);
    }

    inline void schedule(Machine& machine) {
#ifndef DISABLE_POWER_SAVING
        machine.scheduler.schedule(
            constants::sleep_timeout,
            [this, &machine]() {
                this->on_timeout(machine);
            }
        );
#endif
    }
};

class SelectAmount : public StateHandler<State, State::SelectAmount, Context> {
public:
    double amount = 0.0;
public:
    void on_enter(Machine& machine, unsigned long now) override {
        machine.scheduler.schedule(
            constants::timeout,
            [this, &machine, now]() {
                this->on_timeout(machine, now);
            }
        );
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        if (pressed[3] == input::Type::Pressed) {
            machine.transition(State::Home, now);
            return;
        }
        auto button = input::button(pressed);
        amount = constants::dispense_amount[button];
        Serial.println(amount);
        machine.transition(State::SelectRatio, now);
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::select_amount_scene.draw(machine.context.display, now);
    }
private:
    void on_timeout(Machine& machine, unsigned long now) {
        machine.transition(State::Home, now);
    }
};

class SelectRatio : public StateHandler<State, State::SelectRatio, Context> {
public:
    void on_enter(Machine& machine, unsigned long now) override {
        machine.scheduler.schedule(
            constants::timeout,
            [this, &machine, now]() {
                this->on_timeout(machine, now);
            }
        );
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        if (pressed[3] == input::Type::Pressed) {
            machine.transition(State::Home, now);
            return;
        }
        auto button = input::button(pressed);
        ratio = constants::soda_ratio[button];
        Serial.println(machine.get_handler<SelectAmount>().amount);
        machine.transition(State::Dispensing, now);
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::select_ratio_scene.draw(machine.context.display, now);
    }

    double ratio = 0.0;
private:
    void on_timeout(Machine& machine, unsigned long now) {
        machine.transition(State::Home, now);
    }
};

class Dispensing : public StateHandler<State, State::Dispensing, Context> { // TODO add a "menu" to confirm halting dispensing
public:
    void on_enter(Machine& machine, unsigned long now) override {
        dispensed = false;
        // OUT3 and OUT4 on L298N are attached to motor 2 the wrong way around, therefore the LOW, HIGH is reversed to counter
        set_motors<0, 4>({LOW, HIGH, HIGH, LOW});

#ifdef DEBUG_DISPENSE_TIME
        const unsigned long total_time = DEBUG_DISPENSE_TIME;
#else
        const unsigned long total_time = 1.0 / constants::flow_rate * machine.get_handler<SelectAmount>().amount;
#endif
        const unsigned long soda_time = total_time * machine.get_handler<SelectRatio>().ratio;
        const unsigned long lotus_time = total_time - soda_time;

        Serial.println(constants::flow_rate);
        Serial.println(total_time);
        Serial.println(soda_time);
        Serial.println(lotus_time);

        machine.scheduler.schedule(
            soda_time,
            [this, &machine, now]() {
                this->on_dispensed<0, 2>(machine, now);
            }
        );
        machine.scheduler.schedule(
            lotus_time,
            [this, &machine, now]() {
                this->on_dispensed<2, 2>(machine, now);
            }
        );

        auto& dispense = graphics::dispensing_scene.get<custom::Dispense>();
        dispense.start = now;
        dispense.duration = max(soda_time, lotus_time);
        Serial.printf("duration: %lu", dispense.duration);
    }

    bool on_exit(Machine& machine, unsigned long) override {
        RESET_MOTORS
        return true;
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        machine.transition(State::Cancelled, now);
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::dispensing_scene.draw(machine.context.display, now);
    }
private:
    volatile bool dispensed = false; // use atomic?

    template <size_t Start, size_t Count>
    void on_dispensed(Machine& machine, unsigned long now) {
        set_motors<Start, Count>({LOW, LOW});
        if (dispensed)
            machine.transition(State::Leaving, now);
        else dispensed = true;
    }
};

class Leaving : public StateHandler<State, State::Leaving, Context> {
public:
    void on_enter(Machine& machine, unsigned long now) override {
        machine.scheduler.schedule(
            constants::leaving_timeout,
            [this, &machine, now]() {
                this->on_timeout(machine, now);
            }
        );
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::thank_you_scene.draw(machine.context.display, now);
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        machine.transition(State::Home, now);
    }
private:
    void on_timeout(Machine& machine, unsigned long now) {
        machine.transition(State::Home, now);
    }
};

class Cancelled : public StateHandler<State, State::Cancelled, Context> {
public:
    void on_enter(Machine& machine, unsigned long now) override {
        machine.scheduler.schedule(
            constants::cancelled_timeout,
            [this, &machine, now]() {
                this->on_timeout(machine, now);
            }
        );
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::fault_scene.draw(machine.context.display, now);
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        machine.transition(State::Home, now);
    }
private:
    void on_timeout(Machine& machine, unsigned long now) {
        machine.transition(State::Home, now);
    }
};
