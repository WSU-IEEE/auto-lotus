#pragma once

#include "menu.hpp"
#include "util.hpp"
#include "storage.hpp"
#include "logging.hpp"

// TODO dude this is sloppy, make not static, should belong to the menu or something?

namespace lotus {

namespace setup {

struct Settings {
    Flavor flavor = Flavor::Raspberry;
    bool reset_soda = false;
    bool reset_syrup = false;

    void load(Storage& storage) {
        *this = Settings{storage.flavor()};
    }

    void apply(const logging::Logger& logger, Storage& storage) const {
        if (reset_soda) {
            storage.soda_level(constants::soda_quantity);
            LOG_INFO(logger, "reset soda level");
        }
        if (reset_syrup) {
            storage.syrup_level(constants::syrup_quantity);
            LOG_INFO(logger, "reset syrup level");
        }
        if (flavor != storage.flavor()) {
            LOG_INFO(logger, "update selected flavor '%s' -> '%s'", to_string(storage.flavor()).data(), to_string(flavor).data());
            storage.flavor(flavor);
        }
    }
};

class ISetupMenu {
protected:
    Settings m_settings;
    menu::Navigator m_navigator;
    logging::Logger m_logger{"setup_menu"};
public:
    void enter(Storage& storage, unsigned long now) {
        m_settings.load(storage);
        m_navigator.reset(now);
    }

    void apply(Storage& storage) const {
        m_settings.apply(m_logger, storage);
    }

    void handle(const std::array<input::Type, constants::button_pins>& pressed) {
        m_navigator.handle(pressed);
    }

    void draw(U8G2& display) const {
        m_navigator.draw(display);
    }

    void update(unsigned long now) {
        m_navigator.update(now);
    }

    bool closed() const {
        return m_navigator.closed();
    }
};

template <auto Build>
class SetupMenuImpl : public ISetupMenu {
private:
    using Root = decltype(Build(std::declval<Settings&>()));
    Root m_root;
public:
    SetupMenuImpl() : m_root(Build(m_settings)) {
        m_navigator.bind(m_root);
    }
};

inline auto build_menu(Settings& settings) {
    return menu::Menu(
        menu::Dropdown<lotus::Flavor>("Flavor", settings.flavor),
        menu::Action(
            "Purge Soda",
            [](menu::Navigator& navigator) {
                enable_motor<SODA_MOTOR>();
                const auto now = millis();
                navigator.scheduler().schedule(
                    now + constants::purge_time,
                    [](unsigned long) {
                        disable_motor<SODA_MOTOR>();
                    }
                );
            }
        ),
        menu::Action(
            "Purge Syrup",
            [](menu::Navigator& navigator) {
                enable_motor<SYRUP_MOTOR>();
                const auto now = millis();
                navigator.scheduler().schedule(
                    now + constants::purge_time,
                    [](unsigned long) {
                        disable_motor<SYRUP_MOTOR>();
                    }
                );
            }
        ),
        menu::Submenu(
            "Reset Fill Status",
            menu::Menu(
                menu::Action(
                    "Reset Soda Fill",
                    [&settings](menu::Navigator&) {
                        settings.reset_soda = true;
                    }
                ),
                menu::Action(
                    "Reset Syrup Fill",
                    [&settings](menu::Navigator&) {
                        settings.reset_syrup = true;
                    }
                )
            )
        )
    );
}

using SetupMenu = SetupMenuImpl<&build_menu>;

} // namespace setup

} // namespace lotus
