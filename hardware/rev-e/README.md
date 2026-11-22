# DSH-5 Rev E

Rev E puts the microcontroller, audio codec, display support, microSD and 3.3 V supply on the DSH-5 main board. The four-layer, 80 × 176 mm board carries the 30 mm crossfader, level potentiometer, three side switches and two switches for the opposite-side seesaw.

## Hardware

| Reference | Implementation |
| --- | --- |
| U1 | Solder-down ESP32-S3-WROOM-1-N16R8 with 16 MB flash and 8 MB PSRAM. |
| DS1 and J20 | Bare 0.96-inch Winstar OLED with 30-pin FPC and 0.5 mm ZIF connector. The main PCB provides the SSD1306 charge-pump capacitors and current reference. The I²C address is 0x3C. |
| J6 and U2 | JST-XH alternate line input. The main PCB provides ESD protection, 1 kΩ series protection, 100 kΩ bias returns and 2.2 µF AC coupling into the TLV320AIC3204 codec. J11 is the alternative 3.5 mm input. Connect one source at a time. |
| J12 and J13 | Separate 3.5 mm line and headphone outputs from the onboard codec. |
| J21 | Direct-mounted microSD socket on SPI with its own chip select. |
| U14 and U3 | Onboard MCP2518FD CAN-FD controller and TCAN3413 PHY. This preserves the interface to the motor controller. |
| J1 and J2 | USB-C PD charging and internal protected 3S battery. The USB-C connector supplies power. J19 provides the firmware service interface. |

The GL30 motor, moteus-c1 controller and Pololu 12 V regulator remain chassis-mounted. The full BOM includes them. The Fallen CSV excludes the two ready-made chassis boards from the funding request. The bare OLED is a supplier quote item. Blank prices in the CSVs mark items that still need pricing.

## Files

- `kicad/DSH-5-PCB-RevE.kicad_pro` opens the editable schematic and PCB. KiCad 10 standard footprint and 3D model libraries supplement the local project libraries.
- [DSH-5-RevE-PCB.step.xz](../../cad/DSH-5-RevE-PCB.step.xz) contains the populated PCB reference for Fusion. Run `xz -dk cad/DSH-5-RevE-PCB.step.xz` from the repository root to extract it. Its origin is the board's upper-left corner. The OLED panel and motor mount separately in the enclosure. Fit a nonconductive fastener at H2 and keep metal out of the antenna area. Recheck the OLED flex fold, microSD access, service header and headphone cutout with physical samples.
- [BOM.csv](../../BOM.csv) lists components and sourcing links. [FALLEN-BOM.csv](../../FALLEN-BOM.csv) uses the proposal's CSV columns.
- Build the [ESP32-S3 firmware](../../firmware/rev-e/src/) with `pio run` from the repository root. The PlatformIO board manifest configures the compiler and flash settings.

## Verification

The schematic passed KiCad electrical checks. The PCB passed routing and schematic parity checks with zero errors and no unconnected items. KiCad flags the customized ESP32 antenna footprint because its placement rule allows the nearby unplated H2 mounting hole. The area still excludes pads, tracks, vias and copper pours. KiCad also reports an unterminated copper branch with continuity intact. The ESP32-S3 firmware compiled. Physical OLED fit, USB-C charging, audio and motor behavior still need first-article validation.

## Turnkey assembly

Quote both-sided SMT, through-hole controls and connectors, and factory programming. Specify that the assembled board should arrive with all soldering finished. If the order includes the whole deck, quote installing the OLED into J20, mounting the controls and motor stack, and building the keyed chassis harnesses as separate integration operations.

J19 pin order is GND, ESP TX, ESP RX, BOOT, EN, and 3.3 V reference. Use a 3.3 V UART fixture. For download mode, hold BOOT low while cycling EN low then high. Use `pio run -t upload` through the fixture after the board is powered. The USB-C connector is for charging.

[Winstar OLED panel options](https://www.winstar.com.tw/products/oled-module/graphic-oled-display/0_96-oled.html) · [ESP32-S3 module datasheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf) · [TI audio codec](https://www.ti.com/product/TLV320AIC3204) · [PCBWay turnkey assembly quote](https://www.pcbway.com/quotesmt.aspx)
