#pragma once

#include <U8g2lib.h>
#include <utility>

#define BYTE_SIZE 8

struct SpriteView {
    const uint8_t* frames = nullptr;
    u8g2_uint_t width = 0, height = 0;
    size_t count = 1;
    size_t bytes = 0;

    constexpr const uint8_t* frame(size_t i) const {
        return frames + (i % count) * bytes;
    }
};

template <size_t W, size_t H, size_t N>
struct Sprite {
    static constexpr u8g2_uint_t width = static_cast<u8g2_uint_t>(W);
    static constexpr u8g2_uint_t height = static_cast<u8g2_uint_t>(H);
    static constexpr size_t bytes_per_row = (W + BYTE_SIZE - 1) / BYTE_SIZE + 1; // a null terminator is appended too the end of each string so you gotta account for that
    static constexpr size_t bytes = bytes_per_row * H;
    static constexpr size_t count = N;
    uint8_t frames[N][bytes];

    constexpr SpriteView view() const {
        return {
            &frames[0][0],
            static_cast<uint8_t>(W),
            static_cast<uint8_t>(H),
            N,
            bytes
        };
    }
};

namespace element {

struct StaticSprite {
public:
    constexpr StaticSprite(SpriteView sprite, uint8_t bitmap_mode, u8g2_uint_t x, u8g2_uint_t y, uint8_t draw_color)
    : sprite(sprite), x(x), y(y), bitmap_mode(bitmap_mode), draw_color(draw_color)
    { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setBitmapMode(bitmap_mode);
        u8g2.setDrawColor(draw_color);
        u8g2.drawXBMP(x, y, sprite.width, sprite.height, sprite.frames);
    }
private:
    SpriteView sprite;
    uint8_t bitmap_mode;
    // common
    u8g2_uint_t x, y;
    uint8_t draw_color;
};

struct AnimatedSprite {
public:
    constexpr AnimatedSprite(SpriteView sprite, uint8_t bitmap_mode, unsigned long time, unsigned long offset, u8g2_uint_t x, u8g2_uint_t y, uint8_t draw_color)
    : sprite(sprite), bitmap_mode(bitmap_mode), time(time), offset(offset), x(x), y(y), draw_color(draw_color)
    { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setBitmapMode(bitmap_mode);
        u8g2.setDrawColor(draw_color);
        size_t frame = ((now + offset) / time) % sprite.count;
        u8g2.drawXBMP(x, y, sprite.width, sprite.height, sprite.frame(frame));
    }
private:
    SpriteView sprite;
    uint8_t bitmap_mode;
    unsigned long time;
    unsigned long offset;
    // common
    u8g2_uint_t x, y;
    uint8_t draw_color;
};

struct Text {
public:
    constexpr Text(const uint8_t* font, const char* text, uint8_t font_mode, u8g2_uint_t x, u8g2_uint_t y, uint8_t draw_color) :
        font(font), text(text), font_mode(font_mode),
        x(x), y(y), draw_color(draw_color) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setDrawColor(draw_color);
        u8g2.setFontMode(font_mode);
        u8g2.setFont(font);
        u8g2.drawStr(x, y, text);
    }
private:
    const uint8_t* font;
    const char* text;
    uint8_t font_mode;
    // common
    u8g2_uint_t x, y;
    uint8_t draw_color;
};

struct Rectangle {
public:
    constexpr Rectangle(u8g2_uint_t width, u8g2_uint_t height, bool filled, u8g2_uint_t x, u8g2_uint_t y, uint8_t draw_color) :
        width(width), height(height), filled(filled),
        x(x), y(y), draw_color(draw_color) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setDrawColor(draw_color);
        filled ? u8g2.drawBox(x, y, width, height) : u8g2.drawFrame(x, y, width, height);
    }
private:
    u8g2_uint_t width, height;
    bool filled;
    // common
    u8g2_uint_t x, y;
    uint8_t draw_color;
};

} // namespace element

template <typename... E>
struct Scene {
public:
    constexpr explicit Scene(E... elements) : elements(std::move(elements)...) { }

    void draw(U8G2& u8g2, unsigned long now = 0) const {
        iterate(u8g2, now, std::index_sequence_for<E...>{ });
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
