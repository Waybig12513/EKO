# Glasses Project (Nicla Vision + XIAO ESP32-C6)

## Repo layout
- `nicla_vision/firmware/nicla_glasses_fusion/`
  - Arduino IDE sketch + modules (IMU + ToF + optional camera + ML interface + fusion + packet TX)
- `xiao_c6/firmware/`
  - PlatformIO ESP-IDF skeleton (packet RX + haptics later)
- `shared/protocol/`
  - Single source of truth packet format + CRC
- `shared/tools/`
  - Helpers (packet decoder/logging)
- `docs/`
  - LaTeX report template that can include code listings

## Notes for Arduino IDE
Arduino IDE compiles files inside the sketch folder.
For that reason, `hazard_packet.h` and `crc8.h` are duplicated into:
`nicla_vision/firmware/nicla_glasses_fusion/`
(They are also kept in `shared/protocol/` as the canonical copy.)

## Next steps
1) Open `glasses.code-workspace` in VS Code (optional, for navigation).
2) Arduino IDE:
   - Select Nicla Vision board (Arduino Mbed OS Nicla Boards).
   - Install libraries:
     - Arduino_LSM6DSOX
     - VL53L1X (Pololu)
   - Open `nicla_glasses_fusion.ino` and upload.

If camera headers fail to compile on your setup, set `USE_CAMERA` to 0 in `config.h`.
