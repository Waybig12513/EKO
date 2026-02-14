#pragma once
#include <Arduino.h>
#include "vision_types.h"

/*
  vision_inference.h
  Thin interface so you can swap:
    - fake demo outputs (now)
    - real TensorFlow Lite / Edge Impulse inference (later)
*/

bool vision_init();
vision_result_t vision_run(uint16_t tof_mm, float gyro_dps);
