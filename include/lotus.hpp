#pragma once

#include "input.hpp"
#include "logging.hpp"
#include "menu.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "graphics.hpp"
#include "util.hpp"
#include "setup.hpp"
#include "toast.hpp"
#include "logging.hpp"
#include "storage.hpp"

// state machine stuff

namespace lotus {

enum class State {
    Setup,
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
    notification::NotificationManager& notifications;
    Storage& storage;
};

using Machine = state::StateMachine<State, Context>;

class Setup : public state::StateHandler<State, State::Setup, Context> {
private:
    setup::SetupMenu m_menu;
    logging::Logger m_logger{"setup_state"};
public:
    void on_enter(Machine& machine, unsigned long now) override {
        m_menu.enter(machine.context.storage, now);
        LOG_DEBUG(m_logger, "enter handled");
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        m_menu.handle(pressed);
        if (m_menu.closed())
            machine.transition(State::Home, now);
        LOG_DEBUG(m_logger, "input handled");
    }

    void on_update(Machine& machine, unsigned long now) override {
        m_menu.draw(machine.context.display);
        m_menu.update(now);
        //LOG_DEBUG(m_logger, "update handled");
    }

    bool on_exit(Machine& machine, unsigned long now) override {
        disable_motors();
        m_menu.apply(machine.context.storage);
        LOG_DEBUG(m_logger, "exit handled");
        return true;
    }
};

class Home : public state::StateHandler<State, State::Home, Context> {
private:
    volatile bool asleep = false;
    logging::Logger m_logger{"home_state"};
public:
    void on_enter(Machine& machine, unsigned long now) override {
        if (is_soda_low(machine.context.storage.soda_level()))
            machine.context.notifications.send(graphics::soda_low);
        if (is_syrup_low(machine.context.storage.syrup_level()))
            machine.context.notifications.send(graphics::syrup_low);
        asleep = false;
        schedule(machine);
        LOG_DEBUG(m_logger, "enter handled");
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        if (asleep) {
            asleep = false;
            machine.context.display.setPowerSave(0);
            machine.scheduler.reset(now);
            schedule(machine);
        }
        else
            machine.transition(State::SelectAmount, now);
        LOG_DEBUG(m_logger, "input handled");
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::home_scene.draw(machine.context.display, now);
        //LOG_DEBUG(m_logger, "update handled");
    }
private:
    void on_timeout(Machine& machine) {
        LOG_DEBUG(m_logger, "timed out");
        asleep = true;
        machine.context.display.setPowerSave(1);
    }

    inline void schedule(Machine& machine) {
#ifndef DISABLE_POWER_SAVING
        machine.scheduler.schedule(
            constants::sleep_timeout,
            [this, &machine](unsigned long) {
                this->on_timeout(machine);
            }
        );
#endif
    }
};

class SelectAmount : public state::StateHandler<State, State::SelectAmount, Context> {
public:
    double amount = 0.0;
private:
    logging::Logger m_logger{"select_amount_state"};
public:
    void on_enter(Machine& machine, unsigned long now) override {
        machine.scheduler.schedule(
            constants::timeout,
            [this, &machine](unsigned long now) {
                this->on_timeout(machine, now);
            }
        );
        LOG_DEBUG(m_logger, "enter handled");
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        if (pressed[3] == input::Type::Pressed) {
            machine.transition(State::Home, now);
            return;
        }
        auto button = input::button(pressed);
        amount = constants::dispense_amount[button];
        LOG_INFO(m_logger, "amount selected %.2lfml", amount);
        machine.transition(State::SelectRatio, now);
        LOG_DEBUG(m_logger, "handled input");
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::select_amount_scene.draw(machine.context.display, now);
        //LOG_DEBUG(m_logger, "handled update");
    }
private:
    void on_timeout(Machine& machine, unsigned long now) {
        LOG_DEBUG(m_logger, "timed out");
        machine.transition(State::Home, now);
    }
};

class SelectRatio : public state::StateHandler<State, State::SelectRatio, Context> {
public:
    double ratio = 0.0;
private:
    logging::Logger m_logger{"select_ratio_state"};
public:
    void on_enter(Machine& machine, unsigned long now) override {
        machine.scheduler.schedule(
            constants::timeout,
            [this, &machine](unsigned long now) {
                this->on_timeout(machine, now);
            }
        );
        LOG_DEBUG(m_logger, "enter handled");
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        if (pressed[3] == input::Type::Pressed) {
            machine.transition(State::SelectAmount, now);
            return;
        }
        auto button = input::button(pressed);
        ratio = constants::soda_ratio[button];
        LOG_INFO(m_logger, "ratio selected %.2lf%%", ratio * 100);
        machine.transition(State::Dispensing, now);
        LOG_DEBUG(m_logger, "handled input");
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::select_ratio_scene.draw(machine.context.display, now);
        //LOG_DEBUG(m_logger, "handled update");
    }
private:
    void on_timeout(Machine& machine, unsigned long now) {
        machine.transition(State::Home, now);
        LOG_DEBUG(m_logger, "timed out");
    }
};

class Dispensing : public state::StateHandler<State, State::Dispensing, Context> { // TODO add a "menu" to confirm halting dispensing
private:
    volatile bool m_dispensed = false; // use atomic?
    logging::Logger m_logger{"dispensing_state"};
public:
    void on_enter(Machine& machine, unsigned long now) override {
        m_dispensed = false;
        set_motors<0, 2>({HIGH, HIGH});

#ifdef DEBUG_DISPENSE_TIME
        const unsigned long total_time = DEBUG_DISPENSE_TIME;

#else
        const auto amount = machine.get_handler<SelectAmount>().amount;
        const double total_time = 1.0 / constants::flow_rate * amount;
#endif
        const auto ratio = machine.get_handler<SelectRatio>().ratio;
        const unsigned long soda_time = total_time * ratio;
        const unsigned long syrup_time = total_time - soda_time;
        auto duration = max(soda_time, syrup_time);

        LOG_INFO(m_logger, "dispensing with ratio %.2lf%% for %lums, dispense soda for %lums and syrup for %lums", ratio * 100, duration, soda_time, syrup_time);

        machine.scheduler.schedule(
            syrup_time,
            [this, &machine](unsigned long now) {
                disable_motor<SYRUP_MOTOR>();
                LOG_DEBUG(m_logger, "syrup motor disabled");
                this->on_dispensed(machine, now);
            }
        );

        machine.scheduler.schedule(
            soda_time,
            [this, &machine](unsigned long now) {
                disable_motor<SODA_MOTOR>();
                LOG_DEBUG(m_logger, "soda motor disabled");
                this->on_dispensed(machine, now);
            }
        );

        auto& dispense = graphics::dispensing_scene.get<custom::Dispense>();
        dispense.start = now;
        dispense.duration = duration;
        LOG_DEBUG(m_logger, "handeled enter");
    }

    bool on_exit(Machine& machine, unsigned long) override {
        disable_motors();
        LOG_DEBUG(m_logger, "handled exit");
        return true;
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        machine.transition(State::Cancelled, now);
        LOG_DEBUG(m_logger, "handled input");
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::dispensing_scene.draw(machine.context.display, now);
        //LOG_DEBUG(m_logger, "handled update");
    }
private:
    void on_dispensed(Machine& machine, unsigned long now) {
        if (m_dispensed) {
            auto soda_level = machine.context.storage.soda_level();
            auto syrup_level = machine.context.storage.syrup_level();
            const auto amount = machine.get_handler<SelectAmount>().amount;
            const auto ratio = machine.get_handler<SelectRatio>().ratio;
            const auto used_soda = amount * ratio;
            const auto used_syrup = amount - used_soda;
            LOG_INFO(m_logger, "finished dispensing, %.2lfml soda and %.2lfml syrup", used_soda, used_syrup);
            if (soda_level < used_soda)
                machine.context.storage.soda_level(0.0);
            else
                machine.context.storage.soda_level(soda_level - used_soda);
            if (syrup_level < used_syrup)
                machine.context.storage.syrup_level(0.0);
            else
                machine.context.storage.syrup_level(syrup_level - used_syrup);
            LOG_INFO(m_logger, "%.2lfml soda and %.2lfml syrup remaining", machine.context.storage.soda_level(), machine.context.storage.syrup_level());
            machine.transition(State::Leaving, now);
        }
        else
            m_dispensed = true;
    }
};

class Leaving : public state::StateHandler<State, State::Leaving, Context> {
private:
    logging::Logger m_logger{"leaving_state"};
public:
    void on_enter(Machine& machine, unsigned long now) override {
        machine.scheduler.schedule(
            constants::leaving_timeout,
            [this, &machine](unsigned long now) {
                this->on_timeout(machine, now);
            }
        );
        LOG_DEBUG(m_logger, "handled enter");
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::thank_you_scene.draw(machine.context.display, now);
        //LOG_DEBUG(m_logger, "handled update");
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        machine.transition(State::Home, now);
        LOG_DEBUG(m_logger, "handled input");
    }
private:
    void on_timeout(Machine& machine, unsigned long now) {
        LOG_DEBUG(m_logger, "timed out");
        machine.transition(State::Home, now);
    }
};

class Cancelled : public state::StateHandler<State, State::Cancelled, Context> {
private:
    logging::Logger m_logger{"cancelled_state"};
public:
    void on_enter(Machine& machine, unsigned long now) override {
        LOG_INFO(m_logger, "dispensing cancelled!");
        machine.scheduler.schedule(
            constants::cancelled_timeout,
            [this, &machine](unsigned long now) {
                this->on_timeout(machine, now);
            }
        );
        LOG_DEBUG(m_logger, "handled enter");
    }

    void on_update(Machine& machine, unsigned long now) override {
        graphics::fault_scene.draw(machine.context.display, now);
        // LOG_DEBUG(m_logger, "handled update");
    }

    void on_input(Machine& machine, const std::array<input::Type, constants::button_pins>& pressed, unsigned long now) override {
        machine.transition(State::Home, now);
        LOG_DEBUG(m_logger, "handled input");
    }
private:
    void on_timeout(Machine& machine, unsigned long now) {
        LOG_DEBUG(m_logger, "timed out");
        machine.transition(State::Home, now);
    }
};

} // namespace lotus
