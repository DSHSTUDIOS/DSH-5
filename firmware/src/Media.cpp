#include "Media.h"

#include <SD.h>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace {
constexpr unsigned LastRecording = 999;

uint16_t read16(const uint8_t* bytes) {
    return uint16_t(bytes[0]) | (uint16_t(bytes[1]) << 8);
}

uint32_t read32(const uint8_t* bytes) {
    return uint32_t(read16(bytes)) | (uint32_t(read16(bytes + 2)) << 16);
}

void put16(uint8_t* bytes, uint16_t value) {
    bytes[0] = value;
    bytes[1] = value >> 8;
}

void put32(uint8_t* bytes, uint32_t value) {
    put16(bytes, value);
    put16(bytes + 2, value >> 16);
}

void recordName(char (&name)[16], unsigned number, const char* extension) {
    snprintf(name, sizeof(name), "REC%03u.%s", number, extension);
}

unsigned recordingNumber(const char* name) {
    if (strlen(name) != 10 || strncasecmp(name, "REC", 3) != 0 || name[6] != '.') return 0;
    unsigned number = 0;
    for (unsigned i = 3; i < 6; ++i) {
        if (!isdigit(static_cast<unsigned char>(name[i]))) return 0;
        number = number * 10 + name[i] - '0';
    }
    return number;
}

uint32_t loadWave(const char* name, StereoFrame* destination, uint32_t capacity) {
    File file = SD.open(name, FILE_READ);
    uint8_t bytes[16]{};
    if (!file || file.read(bytes, 12) != 12 ||
        memcmp(bytes, "RIFF", 4) || memcmp(bytes + 8, "WAVE", 4)) return 0;

    const uint64_t limit = uint64_t(read32(bytes + 4)) + 8;
    if (limit < 12 || limit > file.size()) return 0;

    bool validFormat = false;
    while (file.position() + 8 <= limit && file.read(bytes, 8) == 8) {
        const uint32_t size = read32(bytes + 4);
        const uint64_t start = file.position();
        const uint64_t next = start + size + (size & 1);
        if (next > limit) return 0;

        if (memcmp(bytes, "fmt ", 4) == 0) {
            if (size < 16 || file.read(bytes, 16) != 16) return 0;
            validFormat = read16(bytes) == 1 && read16(bytes + 2) == 2 &&
                          read32(bytes + 4) == SampleRate && read16(bytes + 14) == 16;
        } else if (memcmp(bytes, "data", 4) == 0 && validFormat) {
            if (size % sizeof(StereoFrame) != 0) return 0;
            const uint32_t frames = min(capacity, size / uint32_t(sizeof(StereoFrame)));
            for (uint32_t offset = 0; offset < frames;) {
                const uint32_t count = min(1024u, frames - offset);
                const size_t chunk = count * sizeof(StereoFrame);
                if (file.read(destination + offset, chunk) != int(chunk)) return 0;
                offset += count;
            }
            return frames;
        }
        if (!file.seek(next)) return 0;
    }
    return 0;
}
}

bool Media::begin() {
    if (!SD.begin(BUILTIN_SDCARD)) return false;
    File root = SD.open("/");
    if (!root) return false;
    for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
        if (entry.isDirectory()) continue;
        const char* name = entry.name();
        const unsigned number = recordingNumber(name);
        if (!number) continue;

        const bool wave = strcasecmp(name + 7, "WAV") == 0;
        const bool temporary = strcasecmp(name + 7, "TMP") == 0;
        if (wave) latestRecording = max(latestRecording, number);
        if (wave || temporary) nextRecording = max(nextRecording, number + 1);
    }
    return true;
}

uint32_t Media::loadBacking(StereoFrame* destination) {
    return loadWave("BACKING.WAV", destination, BackingCapacity);
}

uint32_t Media::loadScratch(StereoFrame* destination) {
    if (latestRecording) {
        char name[16];
        recordName(name, latestRecording, "WAV");
        const uint32_t frames = loadWave(name, destination, ScratchCapacity);
        if (frames) return frames;
    }
    return loadWave("SCRATCH.WAV", destination, ScratchCapacity);
}

bool Media::saveRecording(const StereoFrame* audio, uint32_t frames) {
    if (frames == 0 || frames > ScratchCapacity) return false;
    char name[16];
    char temporary[16];
    do {
        if (nextRecording > LastRecording) return false;
        recordName(name, nextRecording, "WAV");
        recordName(temporary, nextRecording, "TMP");
        ++nextRecording;
    } while (SD.exists(name) || SD.exists(temporary));

    File file = SD.open(temporary, FILE_WRITE_BEGIN);
    if (!file) return false;

    const uint32_t bytes = frames * sizeof(StereoFrame);
    uint8_t header[44]{};
    memcpy(header, "RIFF", 4);
    put32(header + 4, 36 + bytes);
    memcpy(header + 8, "WAVEfmt ", 8);
    put32(header + 16, 16);
    put16(header + 20, 1);
    put16(header + 22, 2);
    put32(header + 24, SampleRate);
    put32(header + 28, SampleRate * sizeof(StereoFrame));
    put16(header + 32, sizeof(StereoFrame));
    put16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    put32(header + 40, bytes);

    bool complete = file.write(header, sizeof(header)) == sizeof(header);
    for (uint32_t offset = 0; complete && offset < frames;) {
        const uint32_t count = min(1024u, frames - offset);
        const size_t chunk = count * sizeof(StereoFrame);
        complete = file.write(reinterpret_cast<const uint8_t*>(audio + offset), chunk) == chunk;
        offset += count;
    }
    file.flush();
    complete = complete && file.size() == sizeof(header) + bytes;
    file.close();
    // An interrupted write stays under its own number so the next take can still save.
    return complete && SD.rename(temporary, name);
}
