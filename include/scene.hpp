#pragma once

#include <U8g2lib.h>
#include <utility>

#define BYTE_SIZE 8

template <size_t W, size_t H, size_t N>
struct Sprite {
    static constexpr u8g2_uint_t width = static_cast<u8g2_uint_t>(W);
    static constexpr u8g2_uint_t height = static_cast<u8g2_uint_t>(H);
    static constexpr size_t bytes_per_row = (W + BYTE_SIZE - 1) / BYTE_SIZE + 1; // a null terminator is appended too the end of each string so you gotta account for that
    static constexpr size_t bytes = bytes_per_row * H;
    static constexpr size_t count = N;
    uint8_t frames[N][bytes];
};

namespace element {

struct StaticSprite {
public:
    template <size_t W, size_t H>
    constexpr StaticSprite(const Sprite<W, H, 1>& sprite, uint8_t bitmapMode, u8g2_uint_t x, u8g2_uint_t y, uint8_t drawColor) :
        bitmap(&sprite.frames[0][0]), width(static_cast<u8g2_uint_t>(W)), height(static_cast<u8g2_uint_t>(H)),
        x(x), y(y), bitmapMode(bitmapMode), drawColor(drawColor) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setBitmapMode(bitmapMode);
        u8g2.setDrawColor(drawColor);
        u8g2.drawXBMP(x, y, width, height, bitmap);
    }
private:
    const uint8_t* bitmap;
    u8g2_uint_t width, height;
    uint8_t bitmapMode;
    // common
    u8g2_uint_t x, y;
    uint8_t drawColor;
};

struct AnimatedSprite {
public:
    template <size_t W, size_t H, size_t N>
    constexpr AnimatedSprite(const Sprite<W, H, N>& sprite, uint8_t bitmapMode, unsigned long time, unsigned long offset, u8g2_uint_t x, u8g2_uint_t y, uint8_t drawColor) :
        frames(&sprite.frames[0][0]), width(static_cast<u8g2_uint_t>(W)), height(static_cast<u8g2_uint_t>(H)), count(N), bytes(Sprite<W, H, N>::bytes), bitmapMode(bitmapMode), time(time), offset(offset),
        x(x), y(y), drawColor(drawColor) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setBitmapMode(bitmapMode);
        u8g2.setDrawColor(drawColor);
        size_t frame = ((now + offset) / time) % count;
        u8g2.drawXBMP(x, y, width, height, frames + (frame * bytes));
    }
private:
    const uint8_t* frames;
    u8g2_uint_t width, height;
    size_t count;
    size_t bytes;
    uint8_t bitmapMode;
    unsigned long time;
    unsigned long offset;
    // common
    u8g2_uint_t x, y;
    uint8_t drawColor;
};

struct Text {
public:
    constexpr Text(const uint8_t* font, const char* text, uint8_t fontMode, u8g2_uint_t x, u8g2_uint_t y, uint8_t drawColor) :
        font(font), text(text), fontMode(fontMode),
        x(x), y(y), drawColor(drawColor) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setDrawColor(drawColor);
        u8g2.setFontMode(fontMode);
        u8g2.setFont(font);
        u8g2.drawStr(x, y, text);
    }
private:
    const uint8_t* font;
    const char* text;
    uint8_t fontMode;
    // common
    u8g2_uint_t x, y;
    uint8_t drawColor;
};

struct Rectangle {
public:
    constexpr Rectangle(u8g2_uint_t width, u8g2_uint_t height, bool filled, u8g2_uint_t x, u8g2_uint_t y, uint8_t drawColor) :
        width(width), height(height), filled(filled),
        x(x), y(y), drawColor(drawColor) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setDrawColor(drawColor);
        filled ? u8g2.drawBox(x, y, width, height) : u8g2.drawFrame(x, y, width, height);
    }
private:
    u8g2_uint_t width, height;
    bool filled;
    // common
    u8g2_uint_t x, y;
    uint8_t drawColor;
};

} // namespace element

template <typename... E>
struct Scene {
public:
    constexpr Scene(E... elements) : elements(elements...) {}

    void draw(U8G2& u8g2, unsigned long now = 0) const {
        iterate(u8g2, now, std::index_sequence_for<E...>{});
    }

    template <typename T>
    T& get() {
        return std::get<T>(elements);
    }
private:
    std::tuple<E...> elements;

    template <size_t... I>
    void iterate(U8G2& u8g2, unsigned long now, std::index_sequence<I...>) const {
        (std::get<I>(elements).draw(u8g2, now), ...);
    }
};
