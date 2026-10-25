# Third-party components

This project uses the ESP32 Arduino framework and the following libraries, installed by PlatformIO under their respective licenses:

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) — FreeBSD license
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) — MIT license
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) — MIT license
- [WiFiManager](https://github.com/tzapu/WiFiManager) — MIT license

The Georgia G bitmap was resized from [FBS Logo Library](https://github.com/Lou34964/FBS-Logo-Library/blob/main/CFB%20Logos/Georgia_Bulldogs_logo-300x300.png). University and team trademarks remain the property of their owners; sharing this firmware does not grant rights to those marks. This is an independent fan project and is not affiliated with the University of Georgia or ESPN.

Game information is fetched from ESPN's public website JSON endpoints. These are not a documented supported API, and no availability or field completeness is guaranteed.

The HTTPS certificate root embedded in `include/feed_ca.h` is a public certificate from the feed's verified certificate chain, not a credential or private key.
