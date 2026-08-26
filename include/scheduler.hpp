#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#define EVENT_QUEUE_SIZE 10

class Scheduler {
public:
    using Callback = std::function<void()>;

    struct Event {
        unsigned long time;
        std::function<void()> callback;
    };

    bool schedule(unsigned long time, Callback callback) {
        if (count >= queue.size()) return false;
        auto pos = std::upper_bound(queue.begin(), queue.begin() + count, time, before);
        std::move_backward(pos, queue.begin() + count, queue.begin() + count + 1);
        *pos = {time, std::move(callback)};
        count++;
        return true;
    }

    void reset() {
        start = millis();
        count = 0;
    }

    void process() {
        const auto now = millis();
        if (count > 0 && now - start >= queue[0].time) {
            auto callback = std::move(queue[0].callback);
            std::move(queue.begin() + 1, queue.begin() + count, queue.begin());
            --count;
            callback();
        };
    }
protected:
    std::array<Event, EVENT_QUEUE_SIZE> queue;
    size_t count = 0;
    unsigned long start = millis();

    static bool before(unsigned long time, const Event& event) {
        return time < event.time;
    }
};
