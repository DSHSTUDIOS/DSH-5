#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "Pins.h"

class Codec {
public:
    bool begin() {
        pinMode(Pins::codecReset, OUTPUT);
        digitalWrite(Pins::codecReset, LOW);
        delay(5);
        digitalWrite(Pins::codecReset, HIGH);
        delay(5);
        if (!page(0) || !write(1, 0x01)) return false;
        delay(5);

        // 44.1 kHz, MCLK = 256 Fs, BCLK and LRCLK supplied by ESP32.
        if (!page(0) || !configure({{4,0x00}, {11,0x81}, {12,0x82}, {13,0x00},
                    {14,0x80}, {18,0x81}, {19,0x82}, {20,0x80}, {27,0x00},
                    {60,0x01}, {61,0x01}})) return false;

        // External 1.8 V rails, IN1 with 20k inputs and common-mode return.
        if (!page(1) || !configure({{1,0x08}, {2,0x00}, {10,0x00}, {12,0x08},
                    {13,0x08}, {14,0x08}, {15,0x08}, {52,0x80}, {54,0x80},
                    {55,0x80}, {57,0x80}, {59,0x00}, {60,0x00}})) return false;
        delay(100);
        if (!page(1) || !write(9, 0x3C)) return false;
        delay(100);
        if (!configure({{16,0x00}, {17,0x00}, {18,0x00}, {19,0x00}})) return false;
        return page(0) && configure({{63,0xD4}, {64,0x00}, {81,0xC0}, {82,0x00}});
    }

private:
    struct Setting { uint8_t reg, value; };
    bool write(uint8_t reg, uint8_t value) {
        Wire.beginTransmission(0x18);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }
    bool page(uint8_t bank) { return write(0, bank); }
    bool configure(std::initializer_list<Setting> settings) {
        for (auto item : settings) if (!write(item.reg, item.value)) return false;
        return true;
    }
};
