#include "sensors_imu.h"
#include <Arduino_LSM6DSOX.h>

bool imu_init() {
  /*
    Initializes the LSM6DSOX IMU.
    Returns true if successful; false otherwise.
  */
  return IMU.begin();
}

bool imu_read_gyro_dps(float &gyro_dps_out) {
  /*
    Reads gx,gy,gz (deg/s) and returns magnitude:
      |g| = sqrt(gx^2 + gy^2 + gz^2)
  */
  float gx, gy, gz;

  if (!IMU.gyroscopeAvailable()) return false;
  if (!IMU.readGyroscope(gx, gy, gz)) return false;

  gyro_dps_out = sqrtf(gx*gx + gy*gy + gz*gz);
  return true;
}
