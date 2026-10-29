# Georgia scoreboard - ESP32-035 3.5-inch migration

This project targets the yellow **ESP32-035** board shown in the owner's photo, with the original ESP32, a 3.5-inch **480 x 320 landscape display**, and **XPT2046 resistive touch**. It supersedes the earlier JC4827W543C migration for this device. Do not use the ESP32-S3 binary or the 4.3-inch case on the ESP32-035.

## Scoreboard

Georgia football, men's basketball and baseball only. Tap the sport title to cycle sports; tap G for the next three published games with dates, Eastern times, stadium/arena and city. The schedule closes after seven seconds. Possession/at-bat uses the sport's ball icon. The last play scrolls, with team-specific TIMEOUT labels when supplied. Timeouts appear as dots; basketball bonus appears under the team; baseball balls, strikes and outs appear beneath the batting team. Venue and city appear below the scores.

## Hardware configuration

| Signal | GPIO |
| --- | --- |
| Shared display/touch SPI SCK / MOSI / MISO | 14 / 13 / 12 |
| Display CS / DC | 15 / 2 |
| Display reset | -1 (no independent pin driven) |
| Backlight | 27 |
| XPT2046 CS / IRQ | 33 / 36 |

The TFT and XPT2046 use **the same TFT_eSPI SPI instance**, with separate chip selects. SPI transactions are enabled. Do not reuse the original 2.8-inch board's separate touch SPI pins. The full frame uses a fixed 16-color palette to fit the original ESP32's internal RAM; PSRAM is not required.

The initial build selects the ST7796-compatible 16-bit SPI driver at 20 MHz. The ST7796-compatible build produced correct landscape output and colors on this board, confirmed by the owner on October 29, 2026. The register reads did not return a useful controller ID, so the exact panel IC remains unconfirmed. Some ESP32-035 examples use different controller names; hardware markings alone do not prove the exact panel driver. Serial startup prints panel registers for diagnosis. Do not claim hardware support until the screen is checked.

Reference: [ESP32-035 hardware mapping](https://github.com/littleCdev/ESP32-035/blob/master/Hardware.md).

## Build and upload

Install PlatformIO and run `pio run -e esp32_035`. Upload using `pio run -e esp32_035 -t upload --upload-port COM_NUMBER`, replacing COM_NUMBER with the actual board's port. Speed defaults to 460800. If automatic download mode fails, hold BOOT and tap RESET, releasing BOOT once writing starts.

Back up the existing 4 MB flash before first upload. Never publish that backup; it can contain saved credentials. The chip must identify as **ESP32**, not ESP32-S3. The earlier connection on COM13 reported ESP32-D0WD-V3 with 4 MB flash, matching the photographed board.

## Wi-Fi and controls

Use a 2.4 GHz network. The new firmware retains the existing `dawgs` preference namespace for Wi-Fi, but uses separate `035` calibration keys. If Wi-Fi is not set up, join **Dawgs-Setup** with **GoDawgs!**, then visit **192.168.4.1**. Credentials stay on the device and are not included in the source.

The first run requests two touch-calibration crosses. Press firmly with a stylus or fingernail. Each point times out after 45 seconds rather than blocking permanently. Hold BOOT for 2.5 seconds to repeat calibration. Serial `CALIBRATE` schedules calibration at the next reset. `INVERT 0` / `INVERT 1` changes display inversion. `STATUS` reports connection and memory information without passwords.

## Printed case

![ESP32-035 case](case/case-preview.png)

The updated body is **125 x 88 x 30 mm**. It retains the red bezel, white GEORGIA/BULLDOGS lettering, black rear and stand, 30-degree tilt back from vertical, filled lower gap, and horizontal 14 x 5.5 mm rear USB-C power port facing straight backward at base level. No stylus holder.

The supplied drawing gives a **101.5 x 55.5 mm** board and **94.5 x 47.9 mm** mounting-hole centers, plus **85 mm** glass width. These replace the superseded JC4827W543C dimensions. Hole diameter (assumed 3.2 mm), glass height/offset, active area, PCB thickness and screen stack height still need physical verification.

Download the [case print files](case/georgia-case-print-files.zip) or read [assembly and PETG support recommendations](case/ASSEMBLY.md). The STLs pass closed-solid checks. Shell/stand and bezel/stand intersections are zero within numerical tolerance; this does not establish hardware fit.

## Validation status - October 29, 2026

- PlatformIO release build passed: 1,155,969 bytes application flash, 50,768 bytes static RAM, and a 76,800-byte runtime palette framebuffer.
- Uploaded to the ESP32-D0WD-V3 / 4 MB ESP32-035 over COM13 at 460800 baud; flash hashes verified.
- The owner confirmed readable 480 x 320 landscape, correct red/black/white colors, sport switching, and G schedule/automatic-return touch controls.
- Serial status confirmed Wi-Fi connected and the football feed returned successfully. No Wi-Fi password is included in these files.
- All four case STLs pass closed, positive-volume solid checks. Case/stand intersections are zero within numerical tolerance.
- Printed fit, glass/PCB stack assumptions, insert fit and adapter retention remain unverified. Live possession/timeout/bonus indicators depend on future live-game feed data and have not all been observed on the new hardware.

ESPN website JSON feeds are unofficial and may omit possession, timeouts, bonus, counts or plays. Missing fields remain unavailable. The Georgia marks belong to their owner; asset attribution is retained in THIRD_PARTY.md.
