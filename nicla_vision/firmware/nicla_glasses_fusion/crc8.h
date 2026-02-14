#pragma once
#include <stdint.h>
#include <stddef.h>

// Simple XOR checksum (demo-friendly, lightweight).
static inline uint8_t crc8_xor(const uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) crc ^= data[i];
  return crc;
}
