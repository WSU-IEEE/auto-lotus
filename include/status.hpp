#pragma once

#include "graphics.hpp"
#include "scene.hpp"
#include <string>
#include <U8g2lib.h>

class WaterFillStatus {
    public:
    void draw_water(U8G2& u8g2, u8g2_uint_t x, u8g2_uint_t y, unsigned long now = 0) const {
        u8g2.setDrawColor(1);
        u8g2.setBitmapMode(1);
        u8g2.drawXBMP(x, y, 13, 13, graphics::letter_W.view().frame(now));
        u8g2.drawBox(x, y, 13, 13);
    }
};

class SyrupFillStatus {
    public:
    void draw_syrup(U8G2& u8g2, u8g2_uint_t x, u8g2_uint_t y, unsigned long now = 0) const {
        u8g2.setDrawColor(1);
        u8g2.setBitmapMode(1);
        u8g2.drawXBMP(x, y, 13, 13, graphics::letter_S.view().frame(now));
        u8g2.drawBox(x, y, 13, 13);
    }
};

class StatusManager {
    std::tuple<WaterFillStatus, SyrupFillStatus> icons;

public:
    StatusManager(WaterFillStatus water_status, SyrupFillStatus syrup_status){
        icons = std::make_tuple(water_status, syrup_status);
    }

    /*
   draw((U8G2& u8g2, unsigned long now = 0) const {
        unsigned int location = 0;
        for (auto& icon : icons) {
            location++;
            icon.draw(u8g2, location * 20, 4, now);
        }
    }
        */
public:
        void draw(U8G2& u8g2, unsigned long now = 0) const {
            std::get<0>(icons).draw_water(u8g2, 4, 4, now);
            std::get<1>(icons).draw_syrup(u8g2, 24, 4, now);
        }
};