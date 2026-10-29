# ESP32-035 hardware verification - October 29, 2026

- ESP32-D0WD-V3, 4 MB flash, serial COM13.
- Complete 4 MB factory backup saved privately outside the distributable project.
- Upload at 460800 baud: all written images hash verified.
- Runtime reports 480 x 320 and DASHBOARD_READY.
- Wi-Fi status connected; football feed returned OK.
- Owner confirmed correct red header, black background, white text, landscape, sport selection and G schedule/return controls.
- Display registers returned ID04=ffffffff and IDD3=00000000, so the exact panel IC is not established by readback. This ST7796-compatible build is visually verified on the pictured board.
- Board dimensions and hole centers follow the supplied 101.5 x 55.5 / 94.5 x 47.9 mm drawing. Meshes validated digitally; printed fit has not been tested.
