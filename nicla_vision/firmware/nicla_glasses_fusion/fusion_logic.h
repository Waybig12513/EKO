#pragma once
#include <Arduino.h>
#include "vision_types.h"

/*
  fusion_logic.h
  Converts (vision + ToF + IMU) into a single urgency score for haptics.
*/

uint8_t fuse_to_urgency_q8(const vision_result_t &v, uint16_t tof_mm, float gyro_dps);
