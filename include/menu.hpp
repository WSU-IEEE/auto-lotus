#pragma once

#include <U8g2lib.h>
#include <array>
#include <string>
#include <utility>
#include <type_traits>

#include "input.hpp"
#include "scheduler.hpp"
#include "util.hpp"
#include "storage.hpp"

namespace menu {

enum class Button : uint8_t {
    Down,
    Up,
    Select,
    Cancel
};

static constexpr inline auto margin = 5;
static constexpr inline auto element_height = 8;
static constexpr inline auto font = u8g2_font_t0_11_tf;

u8g2_uint_t inline draw_element(U8G2& display, u8g2_uint_t offset, bool selected, std::string_view label) {
    display.setFont(font); // get string width will crash if you don't set this first :(
    if (selected) {
        display.setDrawColor(1);
        display.drawBox(margin, offset, display.getUTF8Width(label.data()) + margin * 2, element_height);
    }
    display.setFontMode(1);
    display.setDrawColor(!selected);
    display.drawStr(margin * 2, offset, label.data());
    return element_height;
}

class Navigator;

class IMenu {
public:
    virtual void draw(U8G2& u8g2) const = 0;
    virtual void handle(Button button, Navigator& navigator) = 0;
};

class Navigator {
private:
    static constexpr const inline auto max_depth = 2;

    std::array<IMenu*, max_depth> m_stack{};
    size_t m_size = 1;
    bool m_closed = false;
    Scheduler m_scheduler;
public:
    Navigator() = default;

    explicit Navigator(IMenu& root) {
        bind(root);
    }

    void bind(IMenu& root) {
        m_stack[0] = &root;
        m_size = 1;
        m_closed = false;
    }

    void reset(unsigned long now) {
        m_size = 1;
        m_closed = false;
        m_scheduler.reset(now);
    }

    void handle(const std::array<input::Type, lotus::constants::button_pins>& pressed) {
        size_t i = input::button(pressed);
        current().handle(static_cast<Button>(i), *this);
    }

    void draw(U8G2& display) const {
        current().draw(display);
    }

    void update(unsigned long now) {
        m_scheduler.process(now);
    }

    Scheduler& scheduler() {
        return m_scheduler;
    }

    void push(IMenu& menu) {
        if (m_size < max_depth)
            m_stack[m_size++] = &menu;
    }

    void pop() {
        if (m_size > 1)
            --m_size;
        else
            m_closed = true;
    }

    void close() {
        m_closed = true;
    }

    bool closed() const {
        return m_closed;
    }
private:
    IMenu& current() const {
        return *m_stack[m_size - 1];
    }
};

template <typename T>
class Dropdown {
public:
    static constexpr size_t count = static_cast<size_t>(T::Count);
private:
    T& m_reference;
    T m_selected;
    std::string m_label;
    bool m_editing = false;
public:
    template<typename... O>
    Dropdown(std::string_view label, T& reference)
    : m_label(label), m_reference(reference)
    { }

    bool editing() const {
        return m_editing;
    }

    void select(Navigator&) {
        if (m_editing) {
            m_reference = m_selected;
            m_editing = false;
            return;
        }
        m_selected = m_reference;
        m_editing = true;
    }

    void down() {
        m_selected = static_cast<T>((static_cast<size_t>(m_selected) + 1) % count);
    }

    void up() {
        m_selected = static_cast<T>((static_cast<size_t>(m_selected) + count - 1) % count);
    }

    void cancel() {
        m_editing = false;
    }

    u8g2_uint_t draw(U8G2& display, u8g2_uint_t offset, bool selected) const {
        // label
        display.setFont(font); // get string width will crash if you don't set this first :(
        auto label_width = display.getUTF8Width(m_label.c_str()) + margin * 2;
        if (selected) {
            display.setDrawColor(1);
            display.drawBox(margin, offset, label_width, element_height);
        }
        display.setFontMode(1);
        display.setDrawColor(!selected);
        display.drawStr(margin * 2, offset, m_label.c_str());
        // selected option
        display.setDrawColor(1);
        display.drawStr(label_width + margin * 2, offset, lotus::to_string(m_reference).data());
        // options dropdown
        auto new_offset = offset + element_height;
        if (m_editing) {
            auto width = display.getDisplayWidth() - margin * 2;
            display.setDrawColor(1);
            display.drawFrame(margin, new_offset, width, count * element_height);
            for (size_t j = 0; j < count; ++j) {
                size_t highlighted = static_cast<size_t>(m_selected);
                if (highlighted == j) {
                    display.setDrawColor(1);
                    display.drawBox(margin, new_offset, width, element_height);
                }
                display.setDrawColor(highlighted != j);
                display.drawStr(margin * 2, new_offset, lotus::to_string(static_cast<T>(j)).data());
                new_offset += element_height;
            }
        }
        return new_offset;
    }
};

template <typename T>
struct is_dropdown : std::false_type { };

template <typename T>
struct is_dropdown<Dropdown<T>> : std::true_type { };

class Action {
public:
    using Callback = std::function<void(Navigator&)>;
private:
    std::string m_label;
    Callback m_callback;
public:
    Action(std::string_view label, Callback&& callback)
    : m_label(label), m_callback(std::forward<Callback>(callback))
    { }

    bool editing() const {
        return false;
    }

    void select(Navigator& navigator) {
        m_callback(navigator);
    }

    u8g2_uint_t draw(U8G2& display, size_t i, bool selected) const {
        return draw_element(display, i, selected, m_label);
    }
};

template <typename Menu>
class Submenu {
private:
    std::string m_label;
    Menu m_menu;
public:
    Submenu(std::string_view label, Menu menu)
    : m_label(label), m_menu(std::move(menu))
    { }

    bool editing() const {
        return false;
    }

    void select(Navigator& navigator) {
        navigator.push(m_menu);
    }

    u8g2_uint_t draw(U8G2& display, size_t i, bool selected) const {
        return draw_element(display, i, selected, m_label);
    }
};

template <typename... E>
class Menu : public IMenu {
public:
    static constexpr size_t N = sizeof...(E);
private:
    std::tuple<E...> m_elements;
    size_t m_selected = 0;

    template <size_t... I>
    void draw_each(U8G2& display, std::index_sequence<I...>) const {
        u8g2_uint_t offset = 0;
        (
            (
                offset += std::get<I>(m_elements).draw(display, offset, m_selected == I)
            ),
            ...
        );
    }

    template <typename F, size_t... I>
    void visit_impl(size_t i, F& function, std::index_sequence<I...>) {
        (
            (i == I ? static_cast<void>(function(std::get<I>(m_elements))) : void()),
            ...
        );
    }

    template <typename F>
    void visit(size_t i, F&& function) {
        visit_impl(i, function, std::index_sequence_for<E...>{});
    }
public:
    constexpr explicit Menu(E... elements) : m_elements(std::move(elements)...) { }

    void draw(U8G2& u8g2) const override {
        draw_each(u8g2, std::index_sequence_for<E...>{ });
    }

    void handle(Button button, Navigator& navigator) override {
        bool consumed = false;
        visit(
            m_selected,
            [&](auto& item) {
                if constexpr (is_dropdown<std::decay_t<decltype(item)>>::value) {
                    if(!item.editing())
                        return;
                    consumed = true;
                    switch (button) {
                    case Button::Down:   item.down();   break;
                    case Button::Up:     item.up();     break;
                    case Button::Select: item.select(navigator); break;
                    case Button::Cancel: item.cancel(); break;
                    }
                }
            }
        );
        if (consumed)
            return;
        switch (button) {
        case Button::Down: m_selected = (m_selected + 1) % N;     break;
        case Button::Up:   m_selected = (m_selected + N - 1) % N; break;
        case Button::Select:
            visit(
                m_selected,
                [&](auto& item) {
                    item.select(navigator);
                }
            );
            break;
        case Button::Cancel: navigator.pop(); break;
        }
    }
};

}
