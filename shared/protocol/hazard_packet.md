# Hazard Packet (Nicla -> XIAO)

Purpose: transmit compact metadata (no video) to drive haptics and/or comms.

Fields:
- ver: protocol version (1)
- seq: increments each packet
- t_ms: Nicla timestamp in milliseconds
- class_id: detected object class (0 = none)
- conf_q8: confidence in [0..255]
- dir: -1 left, 0 center, +1 right (from bbox center x)
- urgency_q8: [0..255], higher = stronger/faster vibration
- tof_mm: ToF range in mm (0 if invalid)
- gyro_dps_q8: gyro magnitude (deg/s * 256)
- flags:
  - bit0 turning_fast
  - bit1 tof_valid
  - bit2 vision_valid
- crc8: XOR over bytes [0..(len-2)]
