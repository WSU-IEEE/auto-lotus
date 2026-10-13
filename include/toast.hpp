#pragma once

#include "scene.hpp"

#include <string>
#include <U8g2lib.h>

struct Toast {
    static constexpr size_t label_size = 24;

    struct Icon {
        SpriteView sprite{};
        uint8_t draw_color = 1;
        uint8_t bitmap_mode = 1;
        uint8_t time = static_cast<uint8_t>(300);
        uint8_t offset = 0;
    };

    struct Label {
        const uint8_t* font;
        std::string text;
        uint8_t draw_color = 1;
        uint8_t font_mode = 1;
    };

    Icon icon;
    Label label;

    Toast() = default;

    Toast(Icon&& icon, Label&& label) : icon(std::move(icon)), label(std::move(label)) { }

    void draw_icon(U8G2& u8g2, u8g2_uint_t x, u8g2_uint_t y, unsigned long now = 0) const {
        u8g2.setDrawColor(icon.draw_color);
        u8g2.setBitmapMode(icon.bitmap_mode);
        if (icon.sprite.count > 1) {
            size_t frame = ((now + icon.offset) / icon.time) % icon.sprite.count;
            u8g2.drawXBMP(x, y, icon.sprite.width, icon.sprite.height, icon.sprite.frame(frame));
        } else
            u8g2.drawXBMP(x, y, icon.sprite.width, icon.sprite.height, icon.sprite.frames);
    }

    void draw_label(U8G2& u8g2, u8g2_uint_t x, u8g2_uint_t y, unsigned long now = 0) const {
        u8g2.setDrawColor(label.draw_color);
        u8g2.setFontMode(label.font_mode);
        u8g2.setFont(u8g2_font_t0_11_tf);
        u8g2.drawStr(x, y, label.text.c_str());
    }
};

class NotificationManager {
public:
    U8G2& display;
private:
    const static constexpr inline auto toast_queue_size = 10U;
    const static constexpr inline auto toast_time = 5000U;

    const static constexpr inline auto popup_margin = 20;

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
        draw(m_queue[m_head], now);
    }
private:
    void draw(const Toast& toast, unsigned long now) {
        display.setDrawColor(0);
        display.drawBox(popup_margin, popup_margin, display.getDisplayWidth() - popup_margin, display.getDisplayHeight() - popup_margin);
        display.setDrawColor(1);
        display.drawFrame(popup_margin, popup_margin, display.getDisplayWidth() - popup_margin, display.getDisplayHeight() - popup_margin);
        toast.draw_icon(display, 20, 20, now);
        toast.draw_label(display, 60, 20, now);
    }
};
