#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "Audio.h"
#include "Codec.h"
#include "Pins.h"
#include "Media.h"
#include "Platter.h"

namespace {
Audio deck;
Codec codec;
Adafruit_SSD1306 screen(128,64,&Wire,Pins::oledReset);
Platter platter;
Media media;
Audio::Controls controls;

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
Button rec(Pins::rec), mix(Pins::mix), play(Pins::play);
Button faster(Pins::faster), slower(Pins::slower);

bool audioReady=false, card=false, display=false, memory=false, poweringOff=false, ignorePlay=false;
uint32_t lastDisplay=0, lastControl=0, lastRepeat=0;
float volts=0;
const char* status="Starting";

void initializeMedia() {
    memory = deck.allocate();
    card = memory && media.begin();
    if (!card) {
        status = memory ? "Insert microSD" : "PSRAM unavailable";
        return;
    }

    deck.backingLength = media.loadBacking(deck.backing);
    deck.scratchLength = media.loadScratch(deck.scratch);
    status = deck.backingLength ? "Ready" : "Add BACKING.WAV";
}

void stopRecording() {
    const uint32_t count = deck.stopRecording();
    status = media.saveRecording(deck.scratch, count) ? "Saved scratch" : "SD save failed";
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
    if (deck.recording()) screen.print(" REC");
    else if (controls.scratching) screen.print(" SCRATCH");
    screen.println();
    screen.print("Speed "); screen.print(int(roundf(100*controls.rate))); screen.println("%");
    screen.print(volts,1); screen.print("V ");
    if (digitalRead(Pins::charge)==LOW) screen.print("CHG");
    screen.println();
    screen.println(status);
    if (platter.faulted()) screen.println("Motor fault");
    else if (!platter.healthy()) screen.println("Wheel offline");
    screen.display();
}
}

void setup() {
    for (auto* button : {&rec,&mix,&play,&faster,&slower}) button->begin();
    pinMode(Pins::charge,INPUT_PULLUP);
    for (int cs : {Pins::canCs, Pins::sdCs}) {
        pinMode(cs,OUTPUT);
        digitalWrite(cs,HIGH);
    }
    delay(50);
    analogReadResolution(12);
    SPI.begin(Pins::sck, Pins::miso, Pins::mosi, -1);
    Wire.begin(Pins::sda, Pins::scl, 400000);
    display = screen.begin(SSD1306_SWITCHCAPVCC,0x3C,true,false);
    if (display) {
        screen.setTextWrap(false);
        updateScreen();
    }
    const bool codecReady = deck.clocks() && codec.begin();
    initializeMedia();
    audioReady = codecReady && deck.start();
    if (!audioReady) status = "Audio unavailable";
    platter.begin();
    play.synchronize(millis());
    ignorePlay = play.down;
}
void loop() {
    const uint32_t now=millis();
    platter.poll(audioReady && controls.playing && !controls.scratching && !deck.recording() &&
                 !poweringOff && volts>=9.6f);
    if (now-lastControl>=5) {
        lastControl=now;
        for (auto* button : {&rec,&mix,&play,&faster,&slower}) button->poll(now);
        if (play.down && now-play.heldSince>=900 && !ignorePlay && !poweringOff) {
            poweringOff=true;
            controls.playing=controls.scratching=false;
            deck.update(controls);
            platter.poll(false);
            if (deck.recording()) stopRecording();
            status="Powering off";
        }
        if (play.released) {
            if (audioReady && !ignorePlay && !poweringOff && deck.backingLength) controls.playing=!controls.playing;
            ignorePlay=false;
            poweringOff=false;
        }
        if (audioReady && !poweringOff && rec.pressed) {
            if (deck.recording()) stopRecording();
            else if (memory && card) {
                controls.scratching=false;
                deck.update(controls);
                deck.record();
            } else status=memory ? "Insert microSD" : "PSRAM unavailable";
        }
        if (deck.recordingFull()) stopRecording();
        if (audioReady && !poweringOff && mix.pressed && !deck.recording() && deck.hasScratchSource() && platter.healthy())
            controls.scratching=!controls.scratching;
        if (!platter.healthy()) controls.scratching=false;
        if (faster.down != slower.down && (faster.pressed || slower.pressed || now-lastRepeat>=220)) {
            controls.rate=constrain(controls.rate+(faster.down ? .05f : -.05f),.50f,1.50f);
            lastRepeat=now;
        }
        controls.mix=constrain((analogRead(Pins::fader)-30.f)/4030.f,0.f,1.f);
        controls.level=constrain((analogRead(Pins::level)-30.f)/4030.f,0.f,1.f);
        volts=volts*.85f+(analogReadMilliVolts(Pins::battery)*.001f*(124.9f/24.9f))*.15f;
        controls.scratchRate=platter.velocity();
        deck.update(controls);
    }
    if (now-lastDisplay>=250) {
        lastDisplay=now;
        updateScreen();
    }
}
