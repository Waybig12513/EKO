#pragma once
#include <Arduino.h>
#include "config.h"

/*
  camera_capture.h
  Optional camera wrapper.

  If your environment doesn't have the camera headers available,
  keep USE_CAMERA = 0 in config.h and this module compiles as stubs.
*/

#if USE_CAMERA
  // Some installs expose these with different capitalization; adjust if needed.
  #include "camera.h"
  #ifdef ARDUINO_NICLA_VISION
    #include "gc2145.h"
  #endif
#endif

bool camera_init();
bool camera_grab_frame();
const uint8_t* camera_buffer();
size_t camera_buffer_len();
uint16_t camera_width();
uint16_t camera_height();
