#pragma once

#include <cstdint>

struct StereoFrame {
    int16_t left;
    int16_t right;
};
static_assert(sizeof(StereoFrame) == 4, "WAV frames are two packed 16-bit samples");

constexpr uint32_t SampleRate = 44100;
constexpr uint32_t BackingCapacity = SampleRate * 30;
constexpr uint32_t ScratchCapacity = SampleRate * 8;

class LoopAudio {
public:
    void reset() { position = 0; }

    StereoFrame sample(const StereoFrame* audio, uint32_t frames, float advance) {
        if (!frames) return {};
        const uint32_t index = static_cast<uint32_t>(position);
        const uint32_t next = index + 1 == frames ? 0 : index + 1;
        const float fraction = static_cast<float>(position - index);
        const StereoFrame first = audio[index];
        const StereoFrame second = audio[next];

        // The frame count can exceed a million. Keep fractional motion at either end of the loop.
        position += advance;
        while (position >= frames) position -= frames;
        while (position < 0) position += frames;
        return {
            static_cast<int16_t>(first.left + (second.left - first.left) * fraction),
            static_cast<int16_t>(first.right + (second.right - first.right) * fraction),
        };
    }

private:
    double position = 0;
};
