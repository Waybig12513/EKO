#pragma once
#include <Arduino.h>

/*
  sensors_imu.h
  IMU helpers: init + read gyro magnitude.
*/

bool imu_init();
bool imu_read_gyro_dps(float &gyro_dps_out);
