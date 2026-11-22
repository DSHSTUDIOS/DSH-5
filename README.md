# DSH-5

A handheld DJ deck with a motorized scratch platter, OLED, one crossfader and one level knob. REC, MIX and PLAY sit on the right edge. The opposite edge has a tempo seesaw. Rev D adds onboard USB-C PD charging for an internal battery.

![DSH-5 front and rear studio render](DSH-5-RevD-Studio-Ad.png)

## Rev D files

| Download | Contents |
| --- | --- |
| [KiCad and firmware package](DSH-5-PCB-RevD-Prototype.zip?raw=true) | Editable schematic and routed PCB, Gerbers, drills, BOM, electrical checks, PlatformIO firmware and compiled HEX |
| [CAD package](DSH-5-RevD-CAD.zip?raw=true) | Populated PCB STEP, enclosure parts, internal layout and assembled deck STEP |
| [Blender scene](DSH-5-RevD-Studio.blend?raw=true) | Editable studio scene built from the CAD models |
| [Studio ad](DSH-5-RevD-Studio-Ad.png?raw=true) | 2800 × 1575 PNG |

The [firmware source](firmware), [enclosure generator](cad) and [studio script](studio) are available in the repo. Open `DSH-5-RevD-Deck-Assembly.step` from the CAD package in Fusion to see the enclosure and the populated board together.

KiCad's electrical and routing checks passed, the firmware compiled, and the modeled case cleared the component envelopes, platter and 30 mm fader travel. Hardware bring-up, charging and motor tests, and physical fit of the battery, wiring and rocker remain pending. See the [PCB assembly notes](pcb/README.md) and [CAD fit notes](cad/README.md).
