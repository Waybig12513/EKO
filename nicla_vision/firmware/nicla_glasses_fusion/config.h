#pragma once
/*
  config.h
  Central place for tunables so you don't hunt through the code.
*/

#define USB_SERIAL_BAUD     115200
#define LINK_SERIAL_BAUD    115200

// Rate we send packets to the XIAO (20 Hz = every 50 ms)
#define SEND_PERIOD_MS      50

// Turning detection (from gyro magnitude)
#define TURN_FAST_DPS       120.0f

// ToF thresholds (mm)
#define TOF_NEAR_MM         800
#define TOF_MED_MM          1500
#define TOF_FAR_MM          2500

// Vision stub mode:
// 1 = generate fake detections (lets you demo the pipeline immediately)
// 0 = replace with real model inference later
#define USE_FAKE_VISION     1

// Camera module exists, but is optional.
// If camera headers fail on your setup, keep this at 0.
#define USE_CAMERA          0
