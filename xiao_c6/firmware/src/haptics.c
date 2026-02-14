/*
  haptics.c

  "Policy" layer that converts hazard packets into motor patterns.

  This file does NOT know anything about the camera/model details.
  It only interprets the packet fields:
    - class_id
    - dir
    - urgency_q8
    - confidence_q8
    - turning_fast
    - tof_mm (optional depth hint)

  and outputs easy-to-feel vibration patterns.
*/

#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "haptics/haptics.h"
#include "haptics/motor_driver.h"

static const char *TAG = "haptics";

// -------------------------
// Simple "hazard" to "pattern" mapping
// -------------------------

// This threshold prevents buzzing on low-confidence classifications.
// 0..255 scale (q8 fixed-point)
static const uint8_t kMinConfidenceQ8 = 120;  // ~47%

// If we stop receiving packets, we stop vibrating after this many ms.
static const uint32_t kPacketTimeoutMs = 800;

// How often the haptics task updates the motors.
static const uint32_t kHapticsTickMs = 10;

typedef enum {
  PATTERN_OFF = 0,
  PATTERN_PULSE_ONE,
  PATTERN_PULSE_BOTH,
  PATTERN_CONTINUOUS_BOTH,
} pattern_id_t;

typedef struct {
  pattern_id_t pattern;

  // Which motor(s) to vibrate when using a one-motor pattern.
  // 0=LEFT, 1=CENTER (if enabled), 2=RIGHT
  uint8_t primary_motor;

  // Intensity 0..1
  float intensity;

  // Timing for pulse patterns
  uint32_t period_ms;   // total period
  uint32_t on_ms;       // ON duration inside the period

  // Housekeeping
  uint32_t last_packet_ms;
} haptics_state_t;

static haptics_state_t g_state;
static TaskHandle_t g_task = NULL;

static uint32_t now_ms(void)
{
  // Convert ticks to ms. (Tick rate is typically 1 ms or 10 ms depending on config)
  return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static float q8_to_float(uint8_t v)
{
  return (float)v / 255.0f;
}

static float clamp01(float x)
{
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

static uint8_t pick_primary_motor(const hazard_packet_t *pkt)
{
  // Prefer explicit direction field if present.
  // dir: 0=center/front, 1=left, 2=right
  if (pkt->dir == 1) return 0; // LEFT
  if (pkt->dir == 2) return (uint8_t)(motor_driver_num_motors() - 1); // RIGHT (last motor)

#if HAPTICS_USE_CENTER_MOTOR
  // If we have a center motor, use it for "front/center" hazards.
  return 1; // CENTER
#else
  // Without center motor, default to vibrating BOTH for "front" hazards.
  return 0;
#endif
}

static bool is_dangerous(const hazard_packet_t *pkt)
{
  // A few ways to classify a packet as "danger":
  //  - very high urgency
  //  - very close depth reading
  //  - specific class_id meaning "danger" in your dataset

  const bool urgency_high = pkt->urgency_q8 > 220;
  const bool very_close = (pkt->tof_mm > 0) && (pkt->tof_mm < 300);

  // In your training script you used 5 classes. A common mapping is:
  //   0=clear, 1=front, 2=left, 3=right, 4=danger
  const bool class_is_danger = (pkt->class_id == 4);

  return urgency_high || very_close || class_is_danger;
}

static void apply_pattern_off(void)
{
  g_state.pattern = PATTERN_OFF;
  g_state.intensity = 0.0f;
  motor_all_off();
}

static void compute_pattern_from_packet(const hazard_packet_t *pkt)
{
  // Reject weak predictions so the user isn't constantly buzzing.
  if (pkt->class_id == 0 || pkt->confidence_q8 < kMinConfidenceQ8) {
    apply_pattern_off();
    return;
  }

  // Start with intensity based on urgency.
  float intensity = q8_to_float(pkt->urgency_q8);

  // Use confidence as a "trust" multiplier.
  intensity *= q8_to_float(pkt->confidence_q8);

  // If turning fast, de-emphasize haptics (more false positives while head turns).
  if (pkt->turning_fast) {
    intensity *= 0.6f;
  }

  // Put a floor so low urgency still produces a noticeable pulse.
  if (intensity > 0.0f && intensity < 0.20f) {
    intensity = 0.20f;
  }
  intensity = clamp01(intensity);

  // Translate urgency into pulse timing.
  // Higher urgency => faster pulses.
  // period: 800ms (low) down to 150ms (high)
  const float u = q8_to_float(pkt->urgency_q8);
  const uint32_t period_ms = (uint32_t)(800.0f - (650.0f * u));
  const uint32_t on_ms = (uint32_t)(0.35f * (float)period_ms);

  g_state.intensity = intensity;
  g_state.period_ms = period_ms;
  g_state.on_ms = on_ms;

  // Danger gets special treatment: strong, obvious feedback.
  if (is_dangerous(pkt)) {
    g_state.pattern = PATTERN_PULSE_BOTH;
    g_state.period_ms = 180;
    g_state.on_ms = 120;
    return;
  }

  // Normal obstacles: pulse the "side" motor.
  g_state.pattern = PATTERN_PULSE_ONE;
  g_state.primary_motor = pick_primary_motor(pkt);

  // If we do NOT have a center motor, a "front" hazard should be obvious.
#if !HAPTICS_USE_CENTER_MOTOR
  if (pkt->dir == 0) {
    g_state.pattern = PATTERN_PULSE_BOTH;
  }
#endif
}

void haptics_on_packet(const hazard_packet_t *pkt)
{
  if (!pkt) {
    return;
  }

  g_state.last_packet_ms = now_ms();
  compute_pattern_from_packet(pkt);
}

void haptics_stop(void)
{
  apply_pattern_off();
}

static void drive_one_motor(bool on)
{
  motor_all_off();
  if (on) {
    motor_set_intensity(g_state.primary_motor, g_state.intensity);
  }
}

static void drive_both_motors(bool on)
{
  motor_all_off();
  if (!on) {
    return;
  }

  // LEFT
  motor_set_intensity(0, g_state.intensity);

  // RIGHT is always the last motor in our pin list.
  const uint8_t right = (uint8_t)(motor_driver_num_motors() - 1);
  motor_set_intensity(right, g_state.intensity);

#if HAPTICS_USE_CENTER_MOTOR
  // In danger mode you may also want to vibrate center.
  // (Feel free to comment this out if it’s too "busy")
  if (g_state.pattern == PATTERN_PULSE_BOTH || g_state.pattern == PATTERN_CONTINUOUS_BOTH) {
    motor_set_intensity(1, 0.7f * g_state.intensity);
  }
#endif
}

static void haptics_task(void *arg)
{
  (void)arg;
  ESP_LOGI(TAG, "Haptics task started");

  // Brief self-test buzz so you know motors are wired.
  drive_both_motors(true);
  vTaskDelay(pdMS_TO_TICKS(80));
  motor_all_off();
  vTaskDelay(pdMS_TO_TICKS(120));

  uint32_t phase_ms = 0;
  while (1) {
    const uint32_t t = now_ms();

    // Stop vibrating if packets stopped coming.
    if (g_state.last_packet_ms != 0 && (t - g_state.last_packet_ms) > kPacketTimeoutMs) {
      apply_pattern_off();
    }

    switch (g_state.pattern) {
      case PATTERN_OFF:
        motor_all_off();
        break;

      case PATTERN_PULSE_ONE: {
        // Create a repeating ON/OFF pulse.
        const bool on = (phase_ms < g_state.on_ms);
        drive_one_motor(on);
        break;
      }

      case PATTERN_PULSE_BOTH: {
        const bool on = (phase_ms < g_state.on_ms);
        drive_both_motors(on);
        break;
      }

      case PATTERN_CONTINUOUS_BOTH:
        drive_both_motors(true);
        break;

      default:
        motor_all_off();
        break;
    }

    // Advance phase and wrap around.
    phase_ms += kHapticsTickMs;
    if (g_state.period_ms > 0 && phase_ms >= g_state.period_ms) {
      phase_ms = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(kHapticsTickMs));
  }
}

void haptics_start_task(void)
{
  if (g_task != NULL) {
    return;
  }

  memset(&g_state, 0, sizeof(g_state));
  apply_pattern_off();

  // Pin the task at a modest priority.
  xTaskCreate(haptics_task, "haptics", 4096, NULL, 5, &g_task);
}
