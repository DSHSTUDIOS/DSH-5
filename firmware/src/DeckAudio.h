#pragma once
#include <Audio.h>

struct StereoFrame { int16_t left, right; };
constexpr uint32_t BackingCapacity = 44100u * 30;
constexpr uint32_t ScratchCapacity = 44100u * 8;
extern StereoFrame backing[BackingCapacity], scratch[ScratchCapacity];

class DeckAudio : public AudioStream {
public:
    DeckAudio() : AudioStream(2, input) {}

    struct Controls {
        float mix = 0, level = 0, rate = 1, scratchRate = 0;
        bool playing = false, scratching = false;
    };

    void setControls(const Controls& state) {
        AudioNoInterrupts();
        controls = state;
        AudioInterrupts();
    }

    void record() {
        AudioNoInterrupts();
        captured = 0;
        scratchLength = 0;
        scratchPosition = 0;
        recording = true;
        AudioInterrupts();
    }

    uint32_t stopRecording() {
        AudioNoInterrupts();
        recording = false;
        scratchLength = captured;
        AudioInterrupts();
        return captured;
    }

    volatile bool recording = false;
    volatile uint32_t captured = 0;
    uint32_t scratchLength = 0, backingLength = 0;

    void update() override {
        audio_block_t* inL = receiveReadOnly(0), *inR = receiveReadOnly(1);
        if (recording && inL && inR) {
            for (unsigned i=0; i<AUDIO_BLOCK_SAMPLES && captured<ScratchCapacity; ++i) {
                uint32_t at = captured;
                scratch[at] = {inL->data[i], inR->data[i]};
                captured = at+1;
            }
        }
        if (inL) release(inL);
        if (inR) release(inR);

        auto* outL = allocate();
        auto* outR = allocate();
        if (!outL || !outR) {
            if (outL) release(outL);
            if (outR) release(outR);
            return;
        }
        const Controls c = controls;
        float backTarget = c.playing ? (c.scratching ? 1-c.mix : 1) * c.level : 0;
        float scratchTarget = c.scratching ? c.mix * c.level *
                              constrain(fabsf(c.scratchRate)*20.f,0.f,1.f) : 0;
        for (unsigned i=0; i<AUDIO_BLOCK_SAMPLES; ++i) {
            gainBack += (backTarget-gainBack) * .003f;
            gainScratch += (scratchTarget-gainScratch) * .004f;
            StereoFrame a = sample(backing, backingLength, backPosition, c.playing ? c.rate : 0);
            StereoFrame b = sample(scratch, scratchLength, scratchPosition, c.scratching ? c.scratchRate : 0);
            outL->data[i] = static_cast<int16_t>(constrain(a.left*gainBack+b.left*gainScratch,-32768.f,32767.f));
            outR->data[i] = static_cast<int16_t>(constrain(a.right*gainBack+b.right*gainScratch,-32768.f,32767.f));
        }
        transmit(outL,0); transmit(outR,1);
        release(outL); release(outR);
    }

private:
    audio_block_t* input[2];
    Controls controls;
    float gainBack=0, gainScratch=0, backPosition=0, scratchPosition=0;

    static StereoFrame sample(const StereoFrame* audio, uint32_t frames, float& position, float advance) {
        if (!frames) return {};
        unsigned i = static_cast<unsigned>(position);
        unsigned next = i+1 == frames ? 0 : i+1;
        float bias = position - i;
        StereoFrame a = audio[i], b = audio[next];
        position += advance;
        if (position >= frames) position = fmodf(position,static_cast<float>(frames));
        if (position < 0) position += frames;
        if (position >= frames) position = 0;
        return {static_cast<int16_t>(a.left+(b.left-a.left)*bias),
                static_cast<int16_t>(a.right+(b.right-a.right)*bias)};
    }
};
