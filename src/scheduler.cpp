#include "scheduler.hpp"

#include <Arduino.h>
#include <U8g2lib.h>

bool Scheduler::before(unsigned long time, const Event& event) {
    return time < event.time;
}

bool Scheduler::schedule(unsigned long time, Callback callback) {
    if (m_count >= m_queue.size())
        return false;
    auto pos = std::upper_bound(m_queue.begin(), m_queue.begin() + m_count, time, before);
    std::move_backward(pos, m_queue.begin() + m_count, m_queue.begin() + m_count + 1);
    *pos = {time, std::move(callback)};
    m_count++;
    return true;
}

void Scheduler::reset() {
    m_start = millis();
    m_count = 0;
}

void Scheduler::process() {
    const auto now = millis();
    if (m_count > 0 && now - m_start >= m_queue[0].time) {
        auto callback = std::move(m_queue[0].callback);
        std::move(m_queue.begin() + 1, m_queue.begin() + m_count, m_queue.begin());
        --m_count;
        callback();
    };
}
