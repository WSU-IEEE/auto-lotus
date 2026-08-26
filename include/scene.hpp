#pragma once

#include <U8g2lib.h>

#include <variant>

#define BYTE_SIZE 8

template <size_t W, size_t H, size_t N>
struct Sprite {
    static constexpr u8g2_uint_t width = static_cast<u8g2_uint_t>(W);
    static constexpr u8g2_uint_t height = static_cast<u8g2_uint_t>(H);
    static constexpr size_t count = N;
    static constexpr size_t bytes = W * H / BYTE_SIZE;
    uint8_t frames[N][bytes];
};

struct StaticElement {
public:
    template <size_t W, size_t H>
    constexpr StaticElement(const Sprite<W, H, 1>& sprite, u8g2_uint_t x, u8g2_uint_t y) :
        bitmap(&sprite.frames[0][0]), width(static_cast<u8g2_uint_t>(W)), height(static_cast<u8g2_uint_t>(H)), x(x), y(y) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.drawXBMP(x, y, width, height, bitmap);
    }
private:
    const uint8_t* bitmap;
    u8g2_uint_t width, height;
    u8g2_uint_t x, y;
};

struct AnimatedElement {
public:
    template <size_t W, size_t H, size_t N>
    constexpr AnimatedElement(const Sprite<W, H, N>& sprite, u8g2_uint_t x, u8g2_uint_t y, unsigned long time) :
        frames(&sprite.frames[0][0]), width(static_cast<u8g2_uint_t>(W)), height(static_cast<u8g2_uint_t>(H)), x(x), y(y),
        count(N), bytes(Sprite<W, H, N>::bytes), time(time) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        size_t frame = (now / time) % count;
        u8g2.drawXBMP(x, y, width, height, frames + (frame * bytes));
    }
private:
    const uint8_t* frames;
    u8g2_uint_t width, height;
    u8g2_uint_t x, y;
    size_t count;
    size_t bytes;
    unsigned long time;
};

struct TextElement {
public:
    constexpr TextElement(const uint8_t* font, const char* text, u8g2_uint_t x, u8g2_uint_t y) :
        font(font), text(text), x(x), y(y) { }

    void draw(U8G2& u8g2, unsigned long now) const {
        u8g2.setFont(font);
        u8g2.drawStr(x, y, text);
    }
private:
    const uint8_t* font;
    const char* text;
    u8g2_uint_t x, y;
};

using Element = std::variant<StaticElement, AnimatedElement, TextElement>; // could waste a huge amount of memory if you have large animated bitmaps

template <typename... E>
struct Scene {
public:
    constexpr Scene(E... elements) : elements(elements...) {}

    void draw(U8G2& u8g2, unsigned long now = 0) const {
        std::apply([&u8g2, now](const auto&... element){ (element.draw(u8g2, now), ...); }, elements); // lambda is ugly, use a non/static private member function
    }
private:
    std::tuple<E...> elements;
};

template <typename... E>
Scene(E...) -> Scene<E...>;
