#include "config.h"
#include "vision_inference.h"

/*
  vision_inference.cpp

  This starts with a FAKE vision generator so your end-to-end pipeline works immediately.
  Later you replace vision_run() with real model inference.
*/

bool vision_init() {
  // Placeholder for model init (load TFLite / allocate tensors / etc.).
  return true;
}

static int8_t demo_dir_cycle() {
  /*
    Cycles -1, 0, +1 to demonstrate left/center/right haptics without a model.
  */
  static uint32_t k = 0;
  int8_t seq[3] = {-1, 0, +1};
  int8_t d = seq[k % 3];
  k++;
  return d;
}

vision_result_t vision_run(uint16_t tof_mm, float gyro_dps) {
  vision_result_t r = {0};

#if USE_FAKE_VISION
  // Fake "object detected" if ToF says something is within ~2.5m.
  bool near = (tof_mm > 0 && tof_mm < 2500);

  r.valid = near;
  r.class_id = near ? 1 : 0;    // 1 = "obstacle" placeholder
  r.conf = near ? 0.85f : 0.0f;
  r.dir = near ? demo_dir_cycle() : 0;
#else
  // Real model path (to implement later):
  //  1) capture/crop/resize image
  //  2) normalize pixels
  //  3) run inference
  //  4) map bbox center -> dir, score -> conf, class -> class_id
  r.valid = false;
#endif

  // If user is turning fast, reduce trust slightly (motion blur / instability).
  if (gyro_dps > TURN_FAST_DPS) r.conf *= 0.7f;

  return r;
}
