# DSH-5 Rev D firmware

The firmware runs on a Teensy 4.1 with at least 8 MB of external PSRAM and a PJRC Audio Adapter Rev D2. The [hardware notes](../hardware/README.md) cover the board connections and moteus configuration.

## Build and flash

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) and run `pio run -c firmware/platformio-rev-d.ini` from the repository root. Open [the prebuilt HEX](hex/DSH-5-RevD-Teensy41.hex), or `.pio/build/teensy41/firmware.hex` from your build, in the [Teensy Loader](https://www.pjrc.com/teensy/loader.html). With the board powered, press the Teensy's Program button to upload. Follow the [power wiring notes](../hardware/README.md#electrical-interfaces) before plugging a powered USB cable into an assembled deck.

## Prepare the microSD card

Put a stereo, 16-bit, 44.1 kHz PCM `BACKING.WAV` in the card root. The deck loops the first 30 seconds. The scratch layer uses the latest saved take, then `SCRATCH.WAV` if present, or the backing track itself. Record with REC over line in. A take stops at eight seconds or when you tap REC again.

Completed takes are saved as `REC001.WAV`, `REC002.WAV`, and so on. If power drops mid-save, that take stays under its own `.TMP` filename and later takes can still save. Copy any temporary file off the card before attempting recovery.

The playback and recording code builds locally. Motion, audio latency, charging, and power-loss behavior still need to be verified on the assembled board.
