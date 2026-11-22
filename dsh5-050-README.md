# Rev D enclosure

Run `sh restore-cad.sh` from the repository root, then open `DSH-5-RevD-Deck-Assembly.step` in Fusion for the assembled enclosure and populated PCB. The separate enclosure, PCB and internal-layout files use the same origin and millimeter units.

- `DSH-5-RevD-PCB.step` contains the populated board and the corrected control geometry.
- `DSH-5-RevD-Enclosure.step` contains the chassis, front and rear covers, platter surround, three side buttons, speed seesaw, rear logo inlay and feet.
- `DSH-5-RevD-Internal-Layout.step` contains the mounted motor regulator and volume allowances. These include the battery, Teensy/audio stack, braking resistor and isolated headphone feedthrough.
- `Fit-check.json` records 40 digital interference checks, including the 30 mm fader stroke and platter clearance.

The chassis is 96 × 187 mm, or about 97.9 × 187.1 mm including the edge controls. The main shell is 48 mm deep and the assembled envelope is about 66.4 mm deep from the rear feet to the tallest knob. The battery bay reserves 65 × 37 × 20 mm for a protected 3S pack. Choose and test the actual pack and wire routing before making final tooling. Verify the rocker pivot and button retention under load, fastener engagement, and access with real cables.

To regenerate the CAD, run `python3 cad/build_enclosure.py` from the repository with CadQuery installed. It writes the STEP files and fit report, and tessellates CAD parts into `cad/meshes` for Blender.
