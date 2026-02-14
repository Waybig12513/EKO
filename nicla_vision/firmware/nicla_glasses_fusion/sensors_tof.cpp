#include "sensors_tof.h"
#include <Wire.h>
#include <VL53L1X.h>

static VL53L1X tof;

bool tof_init() {
  /*
    Initializes the onboard VL53L1X ToF sensor.

    Typical Nicla Vision usage:
      Wire1.begin();
      Wire1.setClock(400000);
      tof.setBus(&Wire1);
      tof.startContinuous(...)
  */
  Wire1.begin();
  Wire1.setClock(400000);
  tof.setBus(&Wire1);

  if (!tof.init()) return false;

  tof.setDistanceMode(VL53L1X::Long);
  tof.setMeasurementTimingBudget(50000);
  tof.startContinuous(50);
  return true;
}

bool tof_read_mm(uint16_t &mm_out) {
  /*
    Reads distance in mm.
    Returns false if read is clearly invalid.
  */
  mm_out = (uint16_t)tof.read();
  return (mm_out > 0);
}
