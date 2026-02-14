#pragma once
#include <Arduino.h>

/*
  sensors_tof.h
  ToF helpers using VL53L1X (Pololu) over Wire1 on Nicla Vision.
*/

bool tof_init();
bool tof_read_mm(uint16_t &mm_out);
