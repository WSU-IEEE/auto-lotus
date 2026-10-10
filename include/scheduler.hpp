#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#define EVENT_QUEUE_SIZE 10

class Scheduler {
public:
    const static constexpr inline auto event_queue_size = 10U;

    using Callback = std::function<void()>;

    struct Event {
        unsigned long time;
        std::function<void()> callback;
    };
protected:
    std::array<Event, event_queue_size> m_queue;
    size_t m_count = 0;
    unsigned long m_start = millis();

    static bool before(unsigned long time, const Event& event);
public:
    bool schedule(unsigned long time, Callback callback);

    void reset();

    void process();
};
