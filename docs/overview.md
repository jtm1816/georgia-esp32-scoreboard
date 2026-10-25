# Meet your Georgia scoreboard

A small desk display dedicated to Georgia Bulldogs football, men's basketball, and baseball.

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
| Sport title in the red header | Football → Men's basketball → Baseball → Football |
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

- Use the single-USB ESP32-2432S028 board and a USB power source.
- Connect it to a 2.4 GHz Wi-Fi network. Credentials remain on the device and are excluded from the project source.
- If Wi-Fi setup opens, join **Dawgs-Setup**, use **GoDawgs!**, and visit **192.168.4.1**.
- If taps land in the wrong spot, hold BOOT for 2.5 seconds and press the two calibration crosses firmly.
- See the [project README](../README.md) for building and installing the firmware.

## 3D-printed Georgia case

![Case preview](../case/case-preview.png)

The case has a red bezel with raised white GEORGIA and BULLDOGS lettering, a black rear shell, and a black stand holding the screen **30 degrees back from vertical**. A solid lower support fills the space between the bezel and the flat base.

Four rear M3 screws pass through the board mounting holes into heat-set inserts in the front bezel. The **14 x 5.5 mm USB-C power opening** faces straight backward, parallel to the desk, in a vertical mount resting on the base. The USB-C adapter is for power only.

**Recommended material: PETG** in red, white, and black. Start with 0.20 mm layers, 4 walls, and 20-30% infill. The rear shell needs supports around its projecting USB mount; the other parts can start with supports off. For same-material PETG supports, begin with a 0.30 mm Z gap, 0.6 mm XY separation, and 3 interface layers, then check your slicer's effective settings.

- [Download all case print files and editable design](../case/georgia-case-print-files.zip)
- [Assembly, hardware, and detailed PETG/support settings](../case/ASSEMBLY.md)
- [Individual STL files and editable source](../case)

The files passed digital solid checks. Physical board fit, adapter retention, and print/support settings have not yet been verified on a printed case. Match the M3 insert dimensions to your actual inserts before printing.
