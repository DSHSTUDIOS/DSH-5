#pragma once
#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <cstring>

class Platter {
public:
    void begin() {
        bus.begin();
        CANFD_timings_t timing{};
        timing.clock = CLK_60MHz;
        timing.baudrate = 1000000;
        timing.baudrateFD = 5000000;
        timing.propdelay = 100;
        timing.bus_length = 1;
        timing.sample = 66;
        bus.setBaudRate(timing);
        bus.setRegions(64);
    }

    void poll(bool run) {
        CANFD_message_t message;
        while (bus.read(message)) receive(message);
        uint32_t now = micros();
        if (now - lastQuery < 5000) return;
        lastQuery = now;
        CANFD_message_t frame{};
        frame.id = 0x8001;
        frame.flags.extended = true;
        frame.brs = 1;
        unsigned length = 0;
        bool drive = run && healthy() && !fault;
        if (drive) {
            const uint8_t header[] = {0x01,0x00,0x0a,0x0e,0x20};
            std::memcpy(frame.buf, header, sizeof(header));
            length = sizeof(header);
            putFloat(frame, length, NAN);
            putFloat(frame, length, 0.50f);
            frame.buf[length++] = 0x0d;
            frame.buf[length++] = 0x25;
            putFloat(frame, length, 0.012f);
            frame.buf[length++] = 0x0f;
            frame.buf[length++] = 0x27;
            putFloat(frame, length, 0.045f);
            putFloat(frame, length, 0.60f);
            putFloat(frame, length, 0.80f);
        } else if ((wasDriving || !started) && !fault) {
            frame.buf[length++] = 0x01;
            frame.buf[length++] = 0x00;
            frame.buf[length++] = 0;
        }
        frame.buf[length++] = 0x1e;
        frame.buf[length++] = 0x01;
        frame.buf[length++] = 0x11;
        frame.buf[length++] = 0x0f;
        unsigned wireLength = length;
        for (unsigned legal : {8u,12u,16u,20u,24u,32u,48u,64u}) {
            if (length <= legal) { wireLength = legal; break; }
        }
        while (length < wireLength) frame.buf[length++] = 0x50;
        frame.len = wireLength;
        if (bus.write(frame)) {
            wasDriving = drive;
            started = true;
        }
    }

    bool healthy() const { return seen && micros() - lastReply < 35000 && !fault; }
    float velocity() const { return healthy() ? speed : 0.f; }
    bool faulted() const { return fault; }

private:
    FlexCAN_T4FD<CAN3, RX_SIZE_64, TX_SIZE_16> bus;
    uint32_t lastQuery = 0, lastReply = 0;
    float speed = 0;
    bool wasDriving = false, started = false, seen = false, fault = false;

    static void putFloat(CANFD_message_t& frame, unsigned& length, float value) {
        std::memcpy(frame.buf + length, &value, sizeof(value));
        length += sizeof(value);
    }

    void receive(const CANFD_message_t& frame) {
        if (frame.id != 0x0100 || frame.len < 13) return;
        bool hasPosition = false, hasFault = false;
        float measured = 0;
        uint8_t code = 0;
        for (unsigned i = 0; i < frame.len;) {
            if (frame.buf[i] == 0x50) { ++i; continue; }
            if (i+10 <= frame.len && frame.buf[i] == 0x2e && frame.buf[i+1] == 0x01) {
                float position;
                std::memcpy(&position, frame.buf+i+2, sizeof(position));
                std::memcpy(&measured, frame.buf+i+6, sizeof(measured));
                hasPosition = isfinite(position) && isfinite(measured);
                i += 10;
            } else if (i+3 <= frame.len && frame.buf[i] == 0x21 && frame.buf[i+1] == 0x0f) {
                code = frame.buf[i+2];
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
