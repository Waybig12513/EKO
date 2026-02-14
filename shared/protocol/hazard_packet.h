#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Protocol versioning
#define HAZARD_PKT_VERSION 1

// Flags (bitmask)
#define FLAG_TURNING_FAST (1u << 0)
#define FLAG_TOF_VALID    (1u << 1)
#define FLAG_VISION_VALID (1u << 2)

// Compact packet Nicla -> XIAO
#pragma pack(push, 1)
typedef struct {
  uint8_t  ver;          // protocol version (= HAZARD_PKT_VERSION)
  uint8_t  seq;          // increments each message
  uint32_t t_ms;         // millis() on Nicla
  uint8_t  class_id;     // 0 = none
  uint8_t  conf_q8;      // confidence 0..255
  int8_t   dir;          // -1 left, 0 center, +1 right
  uint8_t  urgency_q8;   // 0..255 (maps to vibration)
  uint16_t tof_mm;       // ToF range in mm (0 if invalid)
  uint16_t gyro_dps_q8;  // gyro magnitude in deg/s * 256
  uint8_t  flags;        // FLAG_* bits
  uint8_t  crc8;         // XOR checksum over all bytes except crc8
} hazard_packet_t;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif
