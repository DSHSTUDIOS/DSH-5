#pragma once
#include <Arduino.h>
#include <ACAN2517FD.h>
#include <SPI.h>
#include "Pins.h"
#include <cstring>

class Platter {
public:
    void begin() {
        ACAN2517FDSettings settings(ACAN2517FDSettings::OSC_40MHz, 1000000,
                                   DataBitRateFactor::x5);
        ready = bus.begin(settings, nullptr) == 0;
    }

    void poll(bool run) {
        if (!ready) return;
        bus.poll();
        CANFDMessage message;
        while (bus.receive(message)) receive(message);
        uint32_t now = micros();
        if (now - lastQuery < 5000) return;
        lastQuery = now;
        CANFDMessage frame{};
        frame.id = 0x8001;
        frame.ext = true;
        frame.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH;
        unsigned length = 0;
        bool drive = run && healthy() && !fault;
        if (drive) {
            const uint8_t header[] = {0x01,0x00,0x0a,0x0e,0x20};
            std::memcpy(frame.data, header, sizeof(header));
            length = sizeof(header);
            putFloat(frame, length, NAN);
            putFloat(frame, length, 0.50f);
            frame.data[length++] = 0x0d;
            frame.data[length++] = 0x25;
            putFloat(frame, length, 0.012f);
            frame.data[length++] = 0x0f;
            frame.data[length++] = 0x27;
            putFloat(frame, length, 0.045f);
            putFloat(frame, length, 0.60f);
            putFloat(frame, length, 0.80f);
        } else if ((wasDriving || !started) && !fault) {
            frame.data[length++] = 0x01;
            frame.data[length++] = 0x00;
            frame.data[length++] = 0;
        }
        frame.data[length++] = 0x1e;
        frame.data[length++] = 0x01;
        frame.data[length++] = 0x11;
        frame.data[length++] = 0x0f;
        unsigned wireLength = length;
        for (unsigned legal : {8u,12u,16u,20u,24u,32u,48u,64u}) {
            if (length <= legal) { wireLength = legal; break; }
        }
        while (length < wireLength) frame.data[length++] = 0x50;
        frame.len = wireLength;
        if (bus.tryToSend(frame)) {
            wasDriving = drive;
            started = true;
        }
    }

    bool healthy() const { return seen && micros() - lastReply < 35000 && !fault; }
    float velocity() const { return healthy() ? speed : 0.f; }
    bool faulted() const { return fault; }

private:
    ACAN2517FD bus{Pins::canCs, SPI, 0xff};
    bool ready = false;
    uint32_t lastQuery = 0, lastReply = 0;
    float speed = 0;
    bool wasDriving = false, started = false, seen = false, fault = false;

    static void putFloat(CANFDMessage& frame, unsigned& length, float value) {
        std::memcpy(frame.data + length, &value, sizeof(value));
        length += sizeof(value);
    }

    void receive(const CANFDMessage& frame) {
        if (frame.id != 0x0100 || frame.len < 13) return;
        bool hasPosition = false, hasFault = false;
        float measured = 0;
        uint8_t code = 0;
        for (unsigned i = 0; i < frame.len;) {
            if (frame.data[i] == 0x50) { ++i; continue; }
            if (i+10 <= frame.len && frame.data[i] == 0x2e && frame.data[i+1] == 0x01) {
                float position;
                std::memcpy(&position, frame.data+i+2, sizeof(position));
                std::memcpy(&measured, frame.data+i+6, sizeof(measured));
                hasPosition = isfinite(position) && isfinite(measured);
                i += 10;
            } else if (i+3 <= frame.len && frame.data[i] == 0x21 && frame.data[i+1] == 0x0f) {
                code = frame.data[i+2];
                hasFault = true;
                i += 3;
            } else {
                return;
            }
        }
        if (hasFault && code) fault = true;
        if (hasPosition && hasFault) {
            speed = constrain(measured, -6.f, 6.f);
            lastReply = micros();
            seen = true;
        }
    }
};
