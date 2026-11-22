#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "DeckAudio.h"
#include "Media.h"
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
Media media;
DeckAudio::Controls controls;

struct Button {
    uint8_t pin;
    bool down=false, raw=false, pressed=false, released=false;
    uint32_t changed=0, heldSince=0;
    explicit Button(uint8_t p):pin(p){}
    void begin() { pinMode(pin,INPUT_PULLUP); }
    void synchronize(uint32_t now) {
        down = raw = digitalRead(pin) == LOW;
        changed = heldSince = now;
    }
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
float volts=0;
const char* status="Starting";

void initializeMedia() {
    memory = external_psram_size >= 8;
    card = memory && media.begin();
    if (!card) {
        status = memory ? "Insert microSD" : "Fit 8MB PSRAM";
        return;
    }

    const uint32_t backingLength = media.loadBacking(backing);
    const uint32_t scratchLength = media.loadScratch(scratch);
    AudioNoInterrupts();
    deck.backingLength = backingLength;
    deck.scratchLength = scratchLength;
    AudioInterrupts();
    status = backingLength ? "Ready" : "Add BACKING.WAV";
}

void stopRecording() {
    const uint32_t count = deck.stopRecording();
    status = media.saveRecording(scratch, count) ? "Saved scratch" : "SD save failed";
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
    screen.println(status);
    if (platter.faulted()) screen.println("Motor fault");
    else if (!platter.healthy()) screen.println("Wheel offline");
    screen.display();
}
}

void setup() {
    for (auto* button : {&rec,&mix,&play,&faster,&slower}) button->begin();
    pinMode(17,INPUT_PULLUP);
    delay(50);
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
    play.synchronize(millis());
    ignorePlay = play.down;
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
        if (!poweringOff && mix.pressed && !deck.recording && deck.hasScratchSource() && platter.healthy())
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
