#pragma once

#include "LoopAudio.h"

class Media {
public:
    bool begin();
    uint32_t loadBacking(StereoFrame* destination);
    uint32_t loadScratch(StereoFrame* destination);
    bool saveRecording(const StereoFrame* audio, uint32_t frames);

private:
    unsigned nextRecording = 1;
    unsigned latestRecording = 0;
};
