# DSH-5 Rev D turnkey PCB assembly

## Quote request

Please quote **one fully assembled first article** of the DSH-5 Rev D main board. List any fabrication minimum separately from the number of boards assembled. The PCB is 80 × 176 mm, four layers and 1.6 mm thick. Quote turnkey component procurement, both-side SMT, through-hole and manual soldering, module installation and prototype inspection. Please include all 118 board positions, including 98 SMT positions and 20 manual or through-hole positions. The customer will receive the board with all soldering completed.

Please flag shortages, substitutions and minimum-buy quantities before sourcing. Report fabrication, components, SMT, manual labor, programming, test, and shipping as separate quote lines. For the two QFN power ICs, quote your usual first-article solder inspection and list X-ray if it is available. Send first-article top and bottom photos and confirm the fitted part numbers.

## Files for the quotation

| File | Use |
| --- | --- |
| [Gerbers and drills](../DSH-5-RevD-Gerbers.zip) | PCB fabrication. Includes both paste layers. |
| [Board-Assembly-BOM.csv](Board-Assembly-BOM.csv) | All 118 board placements. Grouped by part or specification, footprint, side and process. |
| [SMD-placement.csv](../SMD-placement.csv) | Original KiCad XY and rotation export for 98 automated placements, in mm. Bottom-side positions use KiCad coordinates. Confirm orientation in your viewer. |
| [Manual-placement.csv](Manual-placement.csv) | Coordinates for all 20 manual or through-hole positions. Use with the assembly plots and module notes. |
| [Top plot](Assembly-top.svg) and [bottom plot](Assembly-bottom.svg) | Fabrication-layer placement drawings. The bottom drawing is viewed from the underside. |
| [Module-and-harness-BOM.csv](Module-and-harness-BOM.csv) | Stack parts, headers, PSRAM and stereo harness materials. Board positions repeated here identify extra operations and should be counted once. |
| [Schematic](../../DSH-5-RevD-Schematic.pdf) and [hardware notes](../../README.md) | Signal names, module pinouts, electrical interfaces and first-article requirements. |
| [Populated PCB STEP](../../../cad/DSH-5-RevD-PCB.step) | Mechanical reference for the board, controls and connector orientation. |

## Manual assembly and programming

- **Bottom-side Teensy and audio stack.** Solder both 24-pin sockets to the main board and mating pins to the Teensy 4.1. Fit 8 MB of compatible PSRAM to the underside of the Teensy. Fit two 14-pin rows on the PJRC Audio Adapter Rev D2, assemble the stack in the verified orientation and build the J6/J7 jumpers. These jumpers carry L, GND and R from pin 1 through pin 3. Keep the headphone sleeve isolated, per the audio adapter wiring.
- **OLED and regulator.** Fit the Adafruit 326 on the top-side eight-pin interface with the mounting spacers, and the Pololu D24V22F5 below the board on its short header. Check the actual OLED revision, I²C pinout and 0x3D address. Check the Pololu header against PG, EN, VIN, GND and VOUT.
- **Controls and ports.** Hand-fit the five angled switches, linear fader and level pot. Confirm switch clearance and full fader travel. Check shell anchor solder joints on USB-C and the orientation of the two underside stereo jacks.
- **Parts to resolve during DFM.** Confirm the proposed Vishay SSC54-E3/57T for D4 against the SMC pads and polarity. It supersedes the SS54-E3/57T text on the original design. Confirm supply and mechanical fit of the exact RV2 configuration. Any substitute must preserve the 10 kΩ linear function, vertical PCB footprint and shaft geometry. Confirm charger bulk capacitors against the effective capacitance requirements under DC bias.
- **Programming and first-article checks.** Quote programming the Teensy with [the Rev D HEX](../../../firmware/hex/DSH-5-RevD-Teensy41.hex), verifying 8 MB PSRAM, and checking display, switches and analog controls. Follow the [power wiring notes](../../README.md#electrical-interfaces) for USB programming. Quote current-limited power and supervised USB-PD, charger and audio checks separately. The main-board USB-C port supplies power. Program through the Teensy's own USB port.

## If the customer wants a wired deck

Quote the chassis harnesses and integration as a separate operation. That includes the external GL30 motor, moteus-c1 and encoder magnet, 12 V Pololu module, brake resistor, protected 3S pack with its thermistor and the cable assembly. Confirm the battery and encoder fit against the actual parts. The main-board assembly can be quoted while those chassis work items are reviewed.

Rev D is a prototype and still needs first-article electrical and mechanical validation before a production release.

[PCBWay assembly capabilities](https://www.pcbway.com/assembly-capabilities.html)
