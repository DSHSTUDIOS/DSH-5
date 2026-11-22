# DSH-5 Rev D prototype package

Rev D is an **80 × 176 mm, four-layer, 1.6 mm PCB**. It is the first DSH-5 revision with a USB-C receptacle, USB Power Delivery sink, battery charger and system power path on the main board. It retains the direct-mounted fader, level potentiometer, OLED, three right-side switches, and two opposite-side contacts for the seesaw. The Ø54 mm scratch platter uses the GL30 motor and moteus-c1 encoder and drive assembly.

## Electrical interfaces

| Main-board reference | Assembly |
| --- | --- |
| J1 | USB-C power input. Use a PD adapter offering a 12 V or 15 V profile at 1.5 A or more. The hardware gates out default 5 V and 9 V operation. |
| J2 | Pin 1 protected pack positive, pin 2 pack negative. Select a balanced 3S lithium-ion pack rated for 1 A charge and at least 3 A continuous discharge. |
| J3 | Pin 1 NTC, pin 2 ground. Fit a Semitec 103AT-2 bonded to the pack. |
| U4 | Pololu D24V22F5, on the back via a short 2.54 mm header. Powers the Teensy and audio stack. |
| U6 | Four-wire harness to the chassis-mounted Pololu S18V20F12. Pins 1–4 are EN, VIN, GND, VOUT. |
| J18, J17, J4 | J18 motor controller power, VM and GND. J17 CAN H, CAN L, GND. J4 VM and switched return to the chassis braking resistor. |
| J6, J7 | J6 sends stereo line input to the audio shield. J7 brings line output back to the main board. Both headers are L, GND, R. |
| J11, J12 | Board-edge stereo line input and line output. Headphones use the audio shield's insulated stereo jack. Keep the shield's headphone sleeve isolated from system ground. |
| U1 and U2 | Teensy 4.1 with at least 8 MB PSRAM and PJRC Audio Adapter Rev D2. Use the Teensy microSD slot. Separate VUSB and VIN on the Teensy before connecting a powered USB debug cable. |

The charger is hardware-configured for 3S, 12.60 V and a 1.00 A nominal charge current, with temperature qualification and an analog input limit. Charging is independent of the PLAY power latch. The regulated motor output is reverse-isolated from the regulators. The local shunt requires a chassis-mounted 10 Ω, 25 W resistor and starts near 15 V. Configure and calibrate the moteus separately, including current, watchdog timeout behavior, direction, magnet alignment and regenerative power limits. Fit the far-end 120 Ω termination.

## Firmware and controls

The [firmware guide](../firmware/README.md) links the compiled Teensy HEX and explains how to build and flash it. The build used Teensyduino 1.59 and Arm GCC 13.3.1. The project requires at least 8 MB of installed PSRAM on the Teensy. For this build, put a stereo 16-bit, 44.1 kHz PCM `BACKING.WAV` on microSD. Playback loops the first 30 seconds. `SCRATCH.WAV` is optional. REC captures up to eight seconds from line input, saves an incremented WAV and assigns the take to the scratch layer.

| Control | Operation implemented in the build |
| --- | --- |
| PLAY | Short press plays or pauses. Hold to power on or off. |
| REC | Press to record or save a take. |
| MIX | Toggle scratch mode. The motor coasts while encoder movement controls the scratch sample. |
| Front fader | Crossfade between the backing and scratch layers in scratch mode. |
| Front knob | Adjust output level. |
| Speed seesaw | Adjust backing playback rate by 5 points per press. Hold either side to repeat. |
| OLED | Shows deck, recording, battery, charging, and wheel status. |

## Files and remaining verification

The KiCad project contains the routed board and schematic. The [manufacturing folder](manufacturing/) holds prototype Gerbers, drills, a solder paste layer for each side, placement coordinates and a board test netlist. Use the [turnkey quote files](manufacturing/pcba/) for both-side SMT and manual assembly. The [priced BOM](../BOM.csv) includes the separate motor and audio modules. Some passive entries are procurement specifications. Use the specified rating, dielectric, tolerance and footprint and check DC bias for charger capacitors when choosing the exact supplier part.

The [populated PCB STEP](../cad/DSH-5-RevD-PCB.step) uses the original board origin and includes the motor, platter, switch caps, fader, knob, OLED, USB-C and audio ports. The regulator, Teensy and audio shield stack and any harnesses require a physical fit check. The chassis-mounted 12 V regulator's vendor STEP is included separately in [the local KiCad model library](kicad/models/). The [enclosure assembly](../cad) includes component reserves and connector openings. Verify the selected battery, cable bends, headphone and microSD access, and linkage travel with the prototype.

Computer verification covers the schematic, routing, schematic-to-board parity, and the firmware build. Production release requires qualification on assembled hardware. Measure USB-PD attach and charge cutoff with the selected protected pack, charging during playback, both rails at motor startup and stall, reverse motor energy under forced motion, bidirectional scratch latency, recording and shutdown recovery, audio noise, and actual control and connector fit. Test the documented current and thermal limits in the assembled unit. Rev D is a prototype build package pending these checks.

- [Texas Instruments TPS25730D USB-PD sink](https://www.ti.com/product/TPS25730)
- [Texas Instruments BQ25798 power-path charger](https://www.ti.com/product/BQ25798)
- [Pololu motor regulator dimensions](https://www.pololu.com/product/2577/resources)
- [moteus electrical setup and regeneration](https://mjbots.github.io/moteus/guides/electrical-setup/)
- [PJRC Teensy Audio Adapter Rev D2](https://www.pjrc.com/store/teensy3_audio.html)
