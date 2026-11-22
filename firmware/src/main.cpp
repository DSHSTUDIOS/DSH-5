#include <Arduino.h>
#include <Audio.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <array>
#include "DeckAudio.h"
#include "Platter.h"

extern "C" uint8_t external_psram_size;
EXTMEM StereoFrame backing[BackingCapacity], scratch[ScratchCapacity];

namespace {
AudioInputI2S input;
DeckAudio deck;
AudioOutputI2S output;
AudioConnection inputL(input,0,deck,0), inputR(input,1,deck,1);
AudioConnection outputL(deck,0,output,0), outputR(deck,1,output,1);
AudioControlSGTL5000 codec;
Adafruit_SSD1306 screen(128,64,&Wire,-1);
Platter platter;
DeckAudio::Controls controls;

struct Button {
    uint8_t pin;
    bool down=false, raw=false, pressed=false, released=false;
    uint32_t changed=0, heldSince=0;
    explicit Button(uint8_t p):pin(p){}
    void begin() { pinMode(pin,INPUT_PULLUP); }
    void poll(uint32_t now) {
        pressed = released = false;
        const bool sample = digitalRead(pin)==LOW;
        if (sample!=raw) { raw=sample; changed=now; }
        if (raw==down || now-changed<12) return;
        down=raw;
        pressed=down;
        released=!down;
        if (pressed) heldSince=now;
    }
};
Button rec(2), mix(3), play(4), faster(5), slower(6);

bool card=false, display=false, memory=false, poweringOff=false, ignorePlay=false;
uint32_t lastDisplay=0, lastControl=0, lastRepeat=0;
unsigned nextRecording=1;
float volts=0;
const char* status="Starting";

uint16_t u16(const uint8_t* b) { return uint16_t(b[0]) | (uint16_t(b[1])<<8); }
uint32_t u32(const uint8_t* b) { return uint32_t(u16(b)) | (uint32_t(u16(b+2))<<16); }
void put16(uint8_t* b, uint16_t n) { b[0]=n; b[1]=n>>8; }
void put32(uint8_t* b, uint32_t n) { put16(b,n); put16(b+2,n>>16); }

uint32_t loadWave(const char* name, StereoFrame* destination, uint32_t capacity) {
    File file = SD.open(name,FILE_READ);
    uint8_t data[16]{};
    if (!file || file.read(data,12)!=12 || memcmp(data,"RIFF",4) || memcmp(data+8,"WAVE",4)) return 0;
    bool formatOk=false;
    const uint64_t limit = file.size();
    while (file.position()+8 <= limit && file.read(data,8)==8) {
        uint32_t size = u32(data+4);
        uint64_t start = file.position(), next = start + size + (size & 1);
        if (start + size > limit) return 0;
        if (!memcmp(data,"fmt ",4)) {
            if (size<16 || file.read(data,16)!=16) return 0;
            formatOk=u16(data)==1 && u16(data+2)==2 && u32(data+4)==44100 &&
                     u16(data+12)==4 && u16(data+14)==16;
        } else if (!memcmp(data,"data",4) && formatOk) {
            uint32_t frames = min(capacity, size/4);
            for (uint32_t at=0; at<frames;) {
                uint32_t count = min(1024u, frames-at);
                if (file.read(destination+at,count*4)!=int(count*4)) return 0;
                at+=count;
            }
            return frames;
        }
        if (!file.seek(next)) return 0;
    }
    return 0;
}

void recordName(char* name, unsigned number) {
    snprintf(name,16,"REC%03u.WAV",number);
}

void initializeMedia() {
    memory = external_psram_size>=8;
    card = SD.begin(BUILTIN_SDCARD);
    if (!memory || !card) {
        status=!memory ? "Fit 8MB PSRAM" : "Insert microSD";
        return;
    }
    File root = SD.open("/");
    if (root) {
        for (File entry=root.openNextFile(); entry; entry=root.openNextFile()) {
            unsigned number=0;
            char trailing[12]{};
            if (sscanf(entry.name(),"REC%3u.%11s",&number,trailing)==2 &&
                !strcasecmp(trailing,"WAV") && number<1000) {
                nextRecording=max(nextRecording,number+1);
            }
        }
    }
    deck.backingLength=loadWave("BACKING.WAV",backing,BackingCapacity);
    uint32_t count=0;
    if (nextRecording>1) {
        char name[16];
        recordName(name,nextRecording-1);
        count=loadWave(name,scratch,ScratchCapacity);
    }
    deck.scratchLength=count ? count : loadWave("SCRATCH.WAV",scratch,ScratchCapacity);
    status=deck.backingLength ? "Ready" : "Add BACKING.WAV";
}

bool saveRecording(uint32_t frames) {
    if (!card || frames==0 || nextRecording>999) return false;
    char name[16];
    recordName(name,nextRecording);
    if (SD.exists(name)) return false;
    // Preserve an interrupted temporary recording for optional recovery.
    if (SD.exists("CAPTURE.TMP")) return false;
    File file=SD.open("CAPTURE.TMP",FILE_WRITE_BEGIN);
    if (!file) return false;
    uint8_t header[44]{};
    uint32_t bytes=frames*sizeof(StereoFrame);
    memcpy(header,"RIFF",4); put32(header+4,36+bytes); memcpy(header+8,"WAVEfmt ",8);
    put32(header+16,16); put16(header+20,1); put16(header+22,2); put32(header+24,44100);
    put32(header+28,176400); put16(header+32,4); put16(header+34,16);
    memcpy(header+36,"data",4); put32(header+40,bytes);
    bool ok=file.write(header,sizeof(header))==sizeof(header);
    for (uint32_t at=0; ok && at<frames;) {
        uint32_t count=min(1024u,frames-at);
        ok=file.write(reinterpret_cast<const uint8_t*>(scratch+at),count*4)==count*4;
        at+=count;
    }
    file.flush();
    ok=ok && file.size()==44+bytes;
    file.close();
    if (!ok || !SD.rename("CAPTURE.TMP",name)) return false;
    ++nextRecording;
    return true;
}

void stopRecording() {
    const uint32_t count=deck.stopRecording();
    status=saveRecording(count) ? "Saved scratch" : "SD save failed";
}
void updateScreen() {
    if (!display) return;
    screen.clearDisplay();
    screen.setTextColor(SSD1306_WHITE);
    screen.setTextSize(2);
    screen.setCursor(0,0);
    screen.println("DSH-5");
    screen.setTextSize(1);
    screen.print(controls.playing ? "PLAY " : "PAUSE ");
    if (deck.recording) screen.print(" REC");
    else if (controls.scratching) screen.print(" SCRATCH");
    screen.println();
    screen.print("Speed "); screen.print(int(roundf(100*controls.rate))); screen.println("%");
    screen.print(volts,1); screen.print("V ");
    if (digitalRead(17)==LOW) screen.print("CHG");
    screen.println();
    if (platter.faulted()) screen.println("Motor fault");
    else if (!platter.healthy()) screen.println("Wheel offline");
    else screen.println(status);
    screen.display();
}
}

void setup() {
    for (auto* button : {&rec,&mix,&play,&faster,&slower}) button->begin();
    pinMode(17,INPUT_PULLUP);
    delay(50);
    ignorePlay = digitalRead(4)==LOW;
    analogReadResolution(12);
    Wire.begin();
    AudioMemory(16);
    codec.enable();
    codec.inputSelect(AUDIO_INPUT_LINEIN);
    codec.volume(0.42f);
    display=screen.begin(SSD1306_SWITCHCAPVCC,0x3D,false);
    if (display) { screen.setTextWrap(false); updateScreen(); }
    initializeMedia();
    platter.begin();
}
void loop() {
    const uint32_t now=millis();
    platter.poll(controls.playing && !controls.scratching && !deck.recording &&
                 !poweringOff && volts>=9.6f);
    if (now-lastControl>=5) {
        lastControl=now;
        for (auto* button : {&rec,&mix,&play,&faster,&slower}) button->poll(now);
        if (play.down && now-play.heldSince>=900 && !ignorePlay && !poweringOff) {
            poweringOff=true;
            controls.playing=controls.scratching=false;
            deck.setControls(controls);
            platter.poll(false);
            if (deck.recording) stopRecording();
            status="Powering off";
        }
        if (play.released) {
            if (!ignorePlay && !poweringOff && deck.backingLength) controls.playing=!controls.playing;
            ignorePlay=false;
            poweringOff=false;
        }
        if (!poweringOff && rec.pressed) {
            if (deck.recording) stopRecording();
            else if (memory && card) {
                controls.scratching=false;
                deck.setControls(controls);
                deck.record();
            } else status=memory ? "Insert microSD" : "Fit 8MB PSRAM";
        }
        if (deck.recording && deck.captured>=ScratchCapacity) stopRecording();
        if (!poweringOff && mix.pressed && !deck.recording && deck.scratchLength && platter.healthy())
            controls.scratching=!controls.scratching;
        if (!platter.healthy()) controls.scratching=false;
        if (faster.down != slower.down && (faster.pressed || slower.pressed || now-lastRepeat>=220)) {
            controls.rate=constrain(controls.rate+(faster.down ? .05f : -.05f),.50f,1.50f);
            lastRepeat=now;
        }
        controls.mix=constrain((analogRead(A0)-30.f)/4030.f,0.f,1.f);
        controls.level=constrain((analogRead(A1)-30.f)/4030.f,0.f,1.f);
        volts=volts*.85f+(analogRead(A2)*3.3f*(124.9f/24.9f)/4095.f)*.15f;
        controls.scratchRate=platter.velocity();
        deck.setControls(controls);
    }
    if (now-lastDisplay>=250) {
        lastDisplay=now;
        updateScreen();
    }
}
