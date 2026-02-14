#pragma once
#include <stdint.h>

/*
  vision_types.h
  Shared structs used between "vision" and "fusion".
*/

// Output of your ML model (or fake generator).
typedef struct {
  bool    valid;       // true if model produced a usable output
  uint8_t class_id;    // 0 = none
  float   conf;        // 0..1
  int8_t  dir;         // -1 left, 0 center, +1 right
} vision_result_t;
