#pragma once

#include <Arduino.h>
#include <driver/i2s.h>
#include <esp_heap_caps.h>
#include "LoopAudio.h"
#include "Pins.h"

class Audio {
public:
    struct Controls {
        float mix = 0, level = 0, rate = 1, scratchRate = 0;
        bool playing = false, scratching = false;
    };
    StereoFrame* backing = nullptr;
    StereoFrame* scratch = nullptr;
    uint32_t backingLength = 0;
    uint32_t scratchLength = 0;

    bool allocate() {
        backing = static_cast<StereoFrame*>(heap_caps_malloc(BackingCapacity * sizeof(StereoFrame),
                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        scratch = static_cast<StereoFrame*>(heap_caps_malloc(ScratchCapacity * sizeof(StereoFrame),
                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (backing && scratch) return true;
        heap_caps_free(backing);
        heap_caps_free(scratch);
        backing = scratch = nullptr;
        return false;
    }

    bool clocks() {
        i2s_config_t cfg{};
        cfg.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX);
        cfg.sample_rate = SampleRate;
        cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
        cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        cfg.dma_buf_count = 8;
        cfg.dma_buf_len = Frames;
        cfg.use_apll = false; // ESP32-S3 uses its PLL-derived I2S clock.
        cfg.tx_desc_auto_clear = true;
        cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
        cfg.bits_per_chan = I2S_BITS_PER_CHAN_16BIT;
        i2s_pin_config_t pins{};
        pins.mck_io_num = Pins::mclk;
        pins.bck_io_num = Pins::bclk;
        pins.ws_io_num = Pins::lrclk;
        pins.data_out_num = Pins::dac;
        pins.data_in_num = Pins::adc;
        return i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) == ESP_OK &&
               i2s_set_pin(I2S_NUM_0, &pins) == ESP_OK &&
               i2s_zero_dma_buffer(I2S_NUM_0) == ESP_OK;
    }

    bool start() {
        return xTaskCreatePinnedToCore(task, "dsh5-audio", 4096, this, 4, nullptr, 0) == pdPASS;
    }

    void update(const Controls& next) {
        portENTER_CRITICAL(&lock);
        controls = next;
        portEXIT_CRITICAL(&lock);
    }

    void record() {
        portENTER_CRITICAL(&lock);
        captured = 0;
        scratchLength = 0;
        resetScratch = true;
        capturing = true;
        portEXIT_CRITICAL(&lock);
    }

    uint32_t stopRecording() {
        portENTER_CRITICAL(&lock);
        capturing = false;
        scratchLength = captured;
        const uint32_t result = captured;
        resetScratch = true;
        portEXIT_CRITICAL(&lock);
        return result;
    }

    bool recording() {
        portENTER_CRITICAL(&lock);
        const bool result = capturing;
        portEXIT_CRITICAL(&lock);
        return result;
    }

    bool recordingFull() {
        portENTER_CRITICAL(&lock);
        const bool result = capturing && captured >= ScratchCapacity;
        portEXIT_CRITICAL(&lock);
        return result;
    }

    bool hasScratchSource() {
        portENTER_CRITICAL(&lock);
        const bool result = scratchLength || backingLength;
        portEXIT_CRITICAL(&lock);
        return result;
    }

private:
    static constexpr unsigned Frames = 128;
    portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
    Controls controls;
    uint32_t captured = 0;
    bool capturing = false, resetScratch = false;
    LoopAudio backingLoop, scratchLoop;
    float gainBack = 0, gainScratch = 0;

    static void task(void* instance) {
        static_cast<Audio*>(instance)->stream();
    }

    void stream() {
        StereoFrame input[Frames], output[Frames];
        while (true) {
            size_t bytes = 0;
            if (i2s_read(I2S_NUM_0, input, sizeof(input), &bytes, pdMS_TO_TICKS(35)) != ESP_OK ||
                bytes != sizeof(input)) continue;

            portENTER_CRITICAL(&lock);
            const Controls c = controls;
            const uint32_t scratchCount = scratchLength;
            if (resetScratch) {
                scratchLoop.reset();
                resetScratch = false;
            }
            if (capturing && scratch) {
                const size_t count = min(uint32_t(Frames), ScratchCapacity - captured);
                if (count) memcpy(scratch + captured, input, count * sizeof(StereoFrame));
                captured += count;
            }
            portEXIT_CRITICAL(&lock);

            const float backTarget = c.playing ? (c.scratching ? 1 - c.mix : 1) * c.level : 0;
            const float scratchTarget = c.scratching ? c.mix * c.level *
                constrain(fabsf(c.scratchRate)*20.f,0.f,1.f) : 0;
            const StereoFrame* source = scratchCount ? scratch : backing;
            const uint32_t sourceCount = scratchCount ? scratchCount : backingLength;
            for (unsigned i=0; i<Frames; ++i) {
                gainBack += (backTarget-gainBack)*.003f;
                gainScratch += (scratchTarget-gainScratch)*.004f;
                const auto a = backingLoop.sample(backing, backingLength, c.playing ? c.rate : 0);
                StereoFrame b{};
                if (c.scratching) b = scratchLoop.sample(source, sourceCount, c.scratchRate);
                output[i].left = int16_t(constrain(a.left*gainBack + b.left*gainScratch,-32768.f,32767.f));
                output[i].right = int16_t(constrain(a.right*gainBack + b.right*gainScratch,-32768.f,32767.f));
            }
            i2s_write(I2S_NUM_0, output, sizeof(output), &bytes, pdMS_TO_TICKS(30));
        }
    }
};
