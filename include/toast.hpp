#pragma once

#include "scene.hpp"

#include <string>
#include <U8g2lib.h>

struct Toast {
    static constexpr size_t label_size = 24;

    SpriteView sprite{};
    std::string label{};

    Toast() = default;

    Toast(SpriteView sprite, std::string_view label) : sprite(sprite), label(label) { }

    void draw(U8G2& u8g2, unsigned long now = 0) const {
        u8g2.setDrawColor(1);
        u8g2.setBitmapMode(1);
        u8g2.drawXBMP(20, 20, sprite.width, sprite.height, sprite.frame(1)); // add support for animated icons later
        u8g2.setFontMode(1);
        u8g2.setFont(u8g2_font_t0_11_tf);
        u8g2.drawStr(0, 0, label.c_str());
    }
};

class NotificationManager {
public:
    U8G2& display;
private:
    const static constexpr inline auto toast_queue_size = 10U;
    const static constexpr inline auto toast_time = 5000U;

    std::array<Toast, toast_queue_size> m_queue{};
    size_t m_head = 0;
    size_t m_count = 0;

    unsigned long m_start = 0;
    bool m_showing = false;
public:
    NotificationManager(U8G2& display) : display(display) { }

    bool send(const Toast& toast) {
        if (m_count >= toast_queue_size)
            return false;
        m_queue[(m_head + m_count) % toast_queue_size] = toast;
        ++m_count;
        return true;
    }

    void reset() {
        m_head = m_count = 0;
    }

    void process(unsigned long now) {
        if (!m_count)
            return;
        if (!m_showing) {
            m_start = now;
            m_showing = true;
        } else if (now - m_start >= toast_time) {
            m_head = (m_head + 1) % toast_queue_size;
            --m_count;
            m_showing = false;
            return;
        }
        m_queue[m_head].draw(display, now);
    }
};
