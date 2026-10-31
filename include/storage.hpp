#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <optional>

#include "logging.hpp"
#include "util.hpp"

// TODO instead of raw value return object with +, -, +=, /=, etc. overloads?
// TODO track how many drinks have been dispensed

class Storage {
private:
    Preferences m_preferences;
    logging::Logger m_logger{"storage"};
    // cache values in memory
    std::optional<double> m_soda_level;
    std::optional<double> m_syrup_level;
    std::optional<lotus::Flavor> m_flavor;
public:
    bool setup() {
        auto success = m_preferences.begin("settings", false);
        if (!success)
            LOG_ERROR(m_logger, "failed to open namespace!");
        return success;
    }

    double soda_level() {
        if (!m_soda_level.has_value()) {
            if (m_preferences.isKey("soda_level"))
                m_soda_level.emplace(m_preferences.getDouble("soda_level"));
            else {
                m_soda_level.emplace(lotus::constants::soda_quantity);
                m_preferences.putDouble("soda_level", m_soda_level.value());
            }
        }
        return m_soda_level.value();
    }

    bool soda_level(double value) {
        auto success = m_preferences.putDouble("soda_level", value);
        if (success)
            m_soda_level = value;
        else
            LOG_ERROR(m_logger, "failed to write to soda_level key!");
        return success ? true : false;
    }

    double syrup_level() {
        if (!m_syrup_level.has_value()) {
            if (m_preferences.isKey("syrup_level"))
                m_syrup_level.emplace(m_preferences.getDouble("syrup_level"));
            else {
                m_syrup_level.emplace(lotus::constants::syrup_quantity);
                m_preferences.putDouble("syrup_level", m_syrup_level.value());
            }
        }
        return m_syrup_level.value();
    }

    bool syrup_level(double value) {
        auto success = m_preferences.putDouble("syrup_level", value);
        if (success)
            m_syrup_level = value;
        else
            LOG_ERROR(m_logger, "failed to write to syrup_level key!");
        return success ? true : false;
    }

    lotus::Flavor flavor() {
        if (!m_flavor.has_value()) {
            if (m_preferences.isKey("flavor"))
                m_flavor.emplace(static_cast<lotus::Flavor>(m_preferences.getUChar("flavor")));
            else {
                m_flavor.emplace(lotus::Flavor::Pink);
                m_preferences.putUChar("flavor", static_cast<uint8_t>(m_flavor.value()));
            }
        }
        return m_flavor.value();
    }

    bool flavor(lotus::Flavor value) {
        auto success = m_preferences.putUChar("flavor", static_cast<uint8_t>(value));
        if (success)
            m_flavor = value;
        else
            LOG_ERROR(m_logger, "failed to write to flavor key!");
        return success ? true : false;
    }

    void end() {
        m_preferences.end();
    }
};
