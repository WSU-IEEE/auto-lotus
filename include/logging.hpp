#pragma once

// TODO integrate with .platformio/packages/framework-arduinoespressif32/cores/esp32/esp32-hal-log.h instead?

#include <Arduino.h>
#include <string>

#include "ansi.hpp"

#define LOG_DEBUG(logger, ...) logging::debug(logger, __VA_ARGS__)
#define LOG_INFO(logger, ...) logging::info(logger, __VA_ARGS__)
#define LOG_WARN(logger, ...) logging::warn(logger, __VA_ARGS__)
#define LOG_ERROR(logger, ...) logging::error(logger, __VA_ARGS__)
#define LOG_FATAL(logger, ...) logging::fatal(logger, __VA_ARGS__)

namespace logging {

enum class Level : uint8_t {
    Fatal,
    Error,
    Warning,
    Info,
    Debug
};

constexpr bool operator>=(Level lhs, Level rhs) {
    return static_cast<uint8_t>(lhs) >= static_cast<uint8_t>(rhs);
}

inline static Level log_level = Level::Info;

struct Logger {
    const std::string name;
};

template <typename... Args>
void debug(const Logger& logger, const char* format, Args... args) {
    if (log_level >= Level::Debug) {
        Serial.printf("[DEBUG] [%lu] [%s]: ", millis(), logger.name.c_str());
        Serial.printf(format, args...);
        Serial.println();
    }
}

template <typename... Args>
void debug(const char* name, const char* format, Args... args) {
    if (log_level >= Level::Debug) {
        Serial.printf("[DEBUG] [%lu] [%s]: ", millis(), name);
        Serial.printf(format, args...);
        Serial.println();
    }
}

template <typename... Args>
void info(const Logger& logger, const char* format, Args... args) {
    if (log_level >= Level::Info) {
        Serial.printf("[INFO] [%lu] [%s]: ", millis(), logger.name.c_str());
        Serial.printf(format, args...);
        Serial.println();
    }
}

template <typename... Args>
void info(const char* name, const char* format, Args... args) {
    if (log_level >= Level::Info) {
        Serial.printf("[INFO] [%lu] [%s]: ", millis(), name);
        Serial.printf(format, args...);
        Serial.println();
    }
}

template <typename... Args>
void warn(const Logger& logger, const char* format, Args... args) {
    if (log_level >= Level::Warning) {
        Serial.printf(FG_YELLOW "[WARN] [%lu] [%s]: ", millis(), logger.name.c_str());
        Serial.printf(format, args...);
        Serial.println(ANSI_RESET);
    }
}

template <typename... Args>
void warn(const char* name, const char* format, Args... args) {
    if (log_level >= Level::Warning) {
        Serial.printf(FG_YELLOW "[WARN] [%lu] [%s]: ", millis(), name);
        Serial.printf(format, args...);
        Serial.println(ANSI_RESET);
    }
}

template <typename... Args>
void error(const Logger& logger, const char* format, Args... args) {
    if (log_level >= Level::Error) {
        Serial.printf(FG_RED "[ERROR] [%lu] [%s]: ", millis(), logger.name.c_str());
        Serial.printf(format, args...);
        Serial.println(ANSI_RESET);
    }
}

template <typename... Args>
void error(const char* name, const char* format, Args... args) {
    if (log_level >= Level::Error) {
        Serial.printf(FG_RED "[ERROR] [%lu] [%s]: ", millis(), name);
        Serial.printf(format, args...);
        Serial.println(ANSI_RESET);
    }
}

template <typename... Args>
void fatal(const Logger& logger, const char* format, Args... args) {
    if (log_level >= Level::Warning) {
        Serial.printf(FG_RED "[FATAL] [%lu] [%s]: ", millis(), logger.name.c_str());
        Serial.printf(format, args...);
        Serial.println(ANSI_RESET);
        abort();
    }
}

template <typename... Args>
void fatal(const char* name, const char* format, Args... args) {
    if (log_level >= Level::Warning) {
        Serial.printf(FG_RED "[FATAL] [%lu] [%s]: ", millis(), name);
        Serial.printf(format, args...);
        Serial.println(ANSI_RESET);
        abort();
    }
}

}
