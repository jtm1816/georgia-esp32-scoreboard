# Meet your Georgia scoreboard

A 3.5-inch ESP32-035 desk display dedicated to Georgia Bulldogs football, men's basketball, and baseball.

![Example football scoreboard](football-preview.png)

*Illustrative game, player, score, and venue. Live information depends on the game feed.*

## What the screen shows

1. **Red header:** Georgia's G opens the upcoming schedule. The sport title switches sports.
2. **Game status:** quarter and time, half and time, or inning. Football adds down and distance; baseball shows occupied bases when supplied.
3. **Teams and scores:** Georgia stays on top and the opponent below. The football, basketball, or baseball icon marks the team in possession or batting.
4. **Timeout dots:** bright dots mean timeouts remaining; dim dots mean used. These are shown for football and basketball when the feed supplies the count.
5. **Basketball bonus:** a gold BONUS or DOUBLE BONUS label appears below the eligible team's name when reported.
6. **Baseball count:** balls, strikes, and outs sit directly below whichever team is batting.
7. **Location:** stadium or arena first, then city, below the scores. Long locations scroll.
8. **Last Play:** the feed's play description includes player names when supplied. Timeouts appear as **TIMEOUT - GEORGIA** or the opponent's name. A timeout without an identified team simply reads **TIMEOUT**; official or media timeouts are labeled separately. Long descriptions scroll.
9. **Bottom strip:** connection status and time since the last update. Old data is marked stale; possession and other live-only indicators are hidden when stale.

## Two simple touch actions

| Tap | Result |
| --- | --- |
| Sport title in the red header | Football â†’ Men's basketball â†’ Baseball â†’ Football |
| Georgia G | Next three published games for the selected sport, including opponents, dates, Eastern times, venues, and cities |

The schedule automatically closes after **seven seconds**. Tap G again to close it sooner. Changing sports also returns to that sport's scoreboard.

## Which game appears?

On startup, live Georgia games take priority: football first, men's basketball second, baseball third. After you manually choose a sport, it stays selected until the board restarts.

When no game is live, the display shows a recent final (within approximately 18 hours of its listed start) or the next published matchup. If the season has ended and no future schedule is published, the last available result may remain visible. The G schedule list contains future published games only; it never fills gaps with invented fixtures.

## Updates and availability

The board connects directly to Wi-Fi, so no phone or computer needs to stay running. The selected sport checks scores approximately every 30 seconds, while other sports are checked about every 90 seconds. Schedule lists refresh about every 15 minutes. Network delays may lengthen these intervals.

The feed is ESPN's unofficial public website data. Availability varies by game and sport, especially baseball. Missing play-by-play is labeled unavailable; unreported timeouts, bonus, and possession are hidden. Unknown baseball counts show a dash. A basketball possession icon may be unavailable. Game clocks reflect feed updates rather than a continuously synchronized clock.

All times use Eastern time and adjust for daylight saving changes. Unannounced starts show TBD.

## Setup and care

- Use the ESP32-035 with XPT2046 resistive touch and a regulated USB power source.
- Connect it to a 2.4 GHz Wi-Fi network. Credentials remain on the device and are excluded from the project source.
- If Wi-Fi setup opens, join **Dawgs-Setup**, use **GoDawgs!**, and visit **192.168.4.1**.
- Calibrate the two touch crosses on first run; hold BOOT for 2.5 seconds to repeat calibration.
- See the [project README](../README.md) for building and installing the firmware.

## ESP32-035 printed case

![Case preview](../case/case-preview.png)

The 125 x 88 x 30 mm case retains the red bezel, white GEORGIA/BULLDOGS lettering, black rear and stand, 30-degree tilt, filled lower gap, and straight-back 14 x 5.5 mm USB-C power port at base level. No stylus holder is included.

The supplied dimensioned image gives a 101.5 x 55.5 mm board with 94.5 x 47.9 mm mounting-hole centers and 85 mm glass width. The model follows these dimensions. Hole diameter, glass height/offset and screen stack remain provisional until checked on the actual hardware.

[Download the case files](../case/georgia-case-print-files.zip) and [read the assembly / PETG support guide](../case/ASSEMBLY.md). Print the bezel red, lettering white, and shell/stand black in PETG. Start with 0.2 mm layers, four walls, and 20-30% infill. The rear USB mounting pad requires support inspection in the slicer. Physical fit and print settings have not been validated on a printed case.

## Hardware verification

On October 29, 2026 the owner confirmed correct landscape colors and both touch controls on the ESP32-035. The upload verified successfully; Wi-Fi connected and the football feed returned data. Printed case fit is still unverified.
