#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

class Scheduler {
public:
    const static constexpr inline auto event_queue_size = 10U;

    using Callback = std::function<void(unsigned long)>;

    struct Event {
        unsigned long time;
        Callback callback;
    };
protected:
    std::array<Event, event_queue_size> m_queue{};
    size_t m_count = 0;
    unsigned long m_start = 0;

    static bool before(unsigned long time, const Event& event) {
        return time < event.time;
    }
public:
    bool schedule(unsigned long time, Callback callback) {
        if (m_count >= m_queue.size())
            return false;
        auto pos = std::upper_bound(m_queue.begin(), m_queue.begin() + m_count, time, before);
        std::move_backward(pos, m_queue.begin() + m_count, m_queue.begin() + m_count + 1);
        *pos = {time, std::move(callback)};
        m_count++;
        return true;
    }

    void reset(unsigned long now) {
        m_start = now;
        m_count = 0;
    }

    void process(unsigned long now) {
        if (m_count > 0 && now - m_start >= m_queue[0].time) {
            auto callback = std::move(m_queue[0].callback);
            std::move(m_queue.begin() + 1, m_queue.begin() + m_count, m_queue.begin());
            --m_count;
            callback(now);
        };
    }
};
