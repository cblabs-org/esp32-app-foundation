# Hardware Setup Guide

This guide covers everything you need to physically build and first-flash a device
using the ESP32 App Foundation framework.

---

## Supported Hardware

The framework ships with board definitions for two configurations. Both use the same
3.12″ SSD1322 OLED display and the same framework code — only the SPI pin assignments
differ.

| Board | SoC | Flash | PSRAM | USB |
|---|---|---|---|---|
| **ESP32-DevKit V1** | ESP32-WROOM-32 | 4 MB | None | Micro-USB |
| **Waveshare ESP32-S3-Nano** | ESP32-S3 | 8 MB | 8 MB QSPI | USB-C |

The ESP32-S3-Nano is the preferred target for new builds: the extra PSRAM gives
significantly more headroom for TLS, large JSON payloads, and additional modules.

---

## What You'll Need

1. **An ESP32 board** — either of the above, or a compatible variant.
2. **A 3.12″ 256×64 OLED display panel** with the SSD1322 controller. This is a wide,
   ultra-high-contrast monochrome display commonly available from AliExpress or similar
   suppliers. Ensure the board **has an SPI interface** (many SSD1322 modules ship with
   an 8-bit parallel interface that must be converted — see below).
3. **6 short jumper wires** (or solder directly for a more permanent build).
4. **A USB cable** appropriate for your board (Micro-USB or USB-C).
5. **[PlatformIO](https://platformio.org)** installed (via VS Code extension or CLI).
6. **Intermediate soldering skills** if wiring directly to display pads.

---

## Preparing the OLED Display for 4-Wire SPI Mode

> [!WARNING]
> This step is easy to miss. Most SSD1322 display modules ship with **8-bit parallel
> (8080-I) mode** enabled. The framework requires **4-Wire SPI mode**.

The mode is selected by a pair of solder pads (or 0-ohm resistors) on the back of the
display PCB — labelled BS0, BS1, BS2, or similar depending on the supplier. You need to:

1. **Remove** the solder bridge that enables 8-bit mode (typically `BS1` bridged).
2. **Add** a solder bridge that enables SPI mode (typically `BS0` bridged).

The exact pad labels vary by supplier. Check the silkscreen on the back of your board.
The correct SPI configuration is usually documented in the supplier's product listing or
datasheet. If unsure, search for your specific board part number + "SPI mode" or
"BS0 BS1 configuration".

Once in SPI mode, connect it using the wiring tables below.

---

## Wiring Guide

Solder or connect the 6 SPI wires between the display and your ESP32 board.

### ESP32-DevKit V1 (esp32dev)

| OLED Pin | Signal | ESP32-DevKit Pin |
|:---------|:------:|:----------------:|
| 1 VSS / GND | GND | GND |
| 2 VCC | 3.3 V | 3V3 |
| 4 D0 / CLK | SPI CLK | **IO18** |
| 5 D1 / DIN | SPI MOSI | **IO23** |
| 14 D/C# | Data/Command | **IO5** |
| 16 CS# | Chip Select | **IO26** |

### Waveshare ESP32-S3-Nano (esp32s3nano)

| OLED Pin | Signal | ESP32-S3-Nano Pin |
|:---------|:------:|:-----------------:|
| 1 VSS / GND | GND | GND |
| 2 VCC | 3.3 V | 3V3 |
| 4 D0 / CLK | SPI CLK | **IO48** |
| 5 D1 / DIN | SPI MOSI | **IO38** |
| 14 D/C# | Data/Command | **IO9** |
| 16 CS# | Chip Select | **IO10** |

> [!NOTE]
> The MISO pin is not used (the display is write-only). You do not need to wire MISO.
> The display's RES# (reset) pin can be left unconnected — the framework performs a
> software reset via the SSD1322 command set.

---

## Wiring Photos

> [!IMPORTANT]
> **Photos wanted!** If you've built one of these, we'd love to include your wiring
> photo here. Open an issue with your image and we'll add it.

*Placeholder — community photos coming soon.*

---

## First Flash

### 1. Clone the repository

```bash
git clone https://github.com/matthewmcneill/esp32-app-foundation.git
cd esp32-app-foundation
```

### 2. Build and flash the sample app

Flash to your board by specifying the correct PlatformIO environment:

```bash
# ESP32-DevKit V1
pio run -e esp32dev --target upload

# Waveshare ESP32-S3-Nano
pio run -e esp32s3nano --target upload
```

### 3. Upload the filesystem

The web portal SPA assets are stored in LittleFS. Upload them separately:

```bash
# ESP32-DevKit V1
pio run -e esp32dev --target uploadfs

# Waveshare ESP32-S3-Nano
pio run -e esp32s3nano --target uploadfs
```

### 4. Open the serial monitor

```bash
pio device monitor
```

On first boot the device will start in **Access Point mode** (SSID: `ESP32-App-Setup`).
Connect to this AP and navigate to `http://192.168.4.1` to configure WiFi. Once
connected to your network, the device will be accessible at `http://<hostname>.local`.

---

## PlatformIO Environment Reference

Environments are defined in [`platformio.ini`](../../platformio.ini) at the repo root.
The key display pin definitions:

```ini
[env:esp32dev]
board = esp32dev
build_flags =
    -D DISPLAY_CS_PIN=26
    -D DISPLAY_DC_PIN=5
    -D SPI_SCK_PIN=18
    -D SPI_MISO_PIN=19
    -D SPI_MOSI_PIN=23

[env:esp32s3nano]
board = esp32s3box
build_flags =
    -D DISPLAY_CS_PIN=10
    -D DISPLAY_DC_PIN=9
    -D SPI_SCK_PIN=48
    -D SPI_MISO_PIN=47
    -D SPI_MOSI_PIN=38
```

To add support for a different board, duplicate one of these environments and update
the pin definitions to match your hardware.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Display stays blank | Still in 8-bit parallel mode | Check/redo the solder bridge on the display PCB |
| Display shows garbled pixels | Wrong CS or DC pin | Check your wiring against the table for your board |
| Serial monitor shows nothing | Wrong COM port / baud rate | PlatformIO auto-detects; ensure USB drivers are installed |
| Upload fails on ESP32-S3-Nano | Needs boot mode | Hold BOOT button while pressing RST, then release both |
| `pio run` can't find board | PlatformIO platform not installed | Run `pio platform install espressif32` |

---

## Next Steps

- [Architecture Overview](architecture-overview.md) — understand the 3-tier framework design
- [Build System](build-system.md) — advanced PlatformIO configuration
- [Security](security.md) — enable authentication, TLS, and OTA signing
- [Developer README](README.md) — full developer documentation index
