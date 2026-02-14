#include "config.h"
#include "fusion_logic.h"

static float clamp01(float x) {
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

static float tof_to_proximity01(uint16_t mm) {
  /*
    Maps distance (mm) -> proximity in [0..1]
    - very close -> 1
    - far/invalid -> 0
  */
  if (mm == 0) return 0.0f;

  if (mm <= TOF_NEAR_MM) return 1.0f;
  if (mm >= TOF_FAR_MM)  return 0.0f;

  // Linear falloff between near and far.
  float t = (float)(TOF_FAR_MM - mm) / (float)(TOF_FAR_MM - TOF_NEAR_MM);
  return clamp01(t);
}

uint8_t fuse_to_urgency_q8(const vision_result_t &v, uint16_t tof_mm, float gyro_dps) {
  /*
    Simple, explainable fusion:
      urgency = (vision_confidence) * (proximity_from_tof)  (+ turning factor)

    This is transparent and defensible in class:
      - easy to tune
      - easy to test
      - avoids black-box "magic" outside the model
  */
  float prox = tof_to_proximity01(tof_mm);
  float conf = v.valid ? clamp01(v.conf) : 0.0f;

  float u = conf * prox;

  // Turning adds risk: small boost if turning fast.
  if (gyro_dps > TURN_FAST_DPS) u = clamp01(u + 0.15f);

  return (uint8_t)roundf(u * 255.0f);
}
