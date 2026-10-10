#pragma once

#include "scene.hpp"

#include <string>
#include <U8g2lib.h>

class NotificationManager {
public:

};

template <size_t W, size_t H, size_t N>
struct Toast {
    const Sprite<W, H, N>& sprite;
    std::string label;

    constexpr Toast(const Sprite<W, H, N>& sprite, std::string_view label) : sprite(sprite), label(label) { }

    void draw(U8G2& u8g2, unsigned long now = 0) const {
        u8g2.setDrawColor(1);
        u8g2.setBitmapMode(1);
        u8g2.drawXBMP(0, 0, static_cast<u8g2_uint_t>(W), static_cast<u8g2_uint_t>(H), &sprite.frames[0][0]);
        u8g2.setFontMode(1);
        u8g2.setFont(u8g2_font_t0_11_tf);
        u8g2.drawStr(0, 0, label.c_str());
    }
};
