#include "camera_capture.h"

#if USE_CAMERA

#ifdef ARDUINO_NICLA_VISION
static GC2145 galaxyCore;
static Camera cam(galaxyCore);
#define IMAGE_MODE CAMERA_RGB565
#else
#define IMAGE_MODE CAMERA_GRAYSCALE
#endif

static FrameBuffer fb;
static uint16_t w = 0;
static uint16_t h = 0;

bool camera_init() {
  /*
    Initializes camera at 320x240 for a reasonable starting point.
    Real models will crop/resize to the model input (e.g., 96x96 or 160x160).
  */
  if (!cam.begin(CAMERA_R320x240, IMAGE_MODE, 30)) return false;
  w = 320; h = 240;
  return true;
}

bool camera_grab_frame() {
  /*
    Captures a frame into fb. Returns true if success.
  */
  return (cam.grabFrame(fb, 3000) == 0);
}

const uint8_t* camera_buffer() { return fb.getBuffer(); }
size_t camera_buffer_len() { return cam.frameSize(); }
uint16_t camera_width() { return w; }
uint16_t camera_height() { return h; }

#else

// Stubs when camera is disabled.
bool camera_init() { return true; }
bool camera_grab_frame() { return false; }
const uint8_t* camera_buffer() { return nullptr; }
size_t camera_buffer_len() { return 0; }
uint16_t camera_width() { return 0; }
uint16_t camera_height() { return 0; }

#endif
