/*
  haptics.c

  Converts hazard packets into motor vibration patterns.

  Fix applied:
    - pkt->confidence_q8 -> pkt->conf_q8
    - pkt->turning_fast  -> (pkt->flags & FLAG_TURNING_FAST)
*/

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "haptics/haptics.h"
#include "haptics/motor_driver.h"

static const char *TAG = "haptics";

static const uint8_t kMinConfidenceQ8  = 120;   // ~47%
static const uint32_t kPacketTimeoutMs = 800;
static const uint32_t kHapticsTickMs   = 10;

typedef enum {
  PATTERN_OFF = 0,
  PATTERN_PULSE_ONE,
  PATTERN_PULSE_BOTH,
  PATTERN_CONTINUOUS_BOTH,
} pattern_id_t;

typedef struct {
  pattern_id_t pattern;
  uint8_t      primary_motor;
  float        intensity;
  uint32_t     period_ms;
  uint32_t     on_ms;
  uint32_t     last_packet_ms;
} haptics_state_t;

static haptics_state_t g_state;
static TaskHandle_t    g_task = NULL;

static uint32_t now_ms(void)
{
  return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static float q8_to_float(uint8_t v)   { return (float)v / 255.0f; }
static float clamp01(float x)
{
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

static uint8_t pick_primary_motor(const hazard_packet_t *pkt)
{
  if (pkt->dir == -1) return 0;  // LEFT
  if (pkt->dir ==  1) return (uint8_t)(motor_driver_num_motors() - 1);  // RIGHT
#if HAPTICS_USE_CENTER_MOTOR
  return 1;  // CENTER
#else
  return 0;
#endif
}

static bool is_dangerous(const hazard_packet_t *pkt)
{
  const bool urgency_high  = pkt->urgency_q8 > 220;
  const bool very_close    = (pkt->tof_mm > 0) && (pkt->tof_mm < 300);
  const bool class_danger  = (pkt->class_id == 4);
  return urgency_high || very_close || class_danger;
}

static void apply_pattern_off(void)
{
  g_state.pattern   = PATTERN_OFF;
  g_state.intensity = 0.0f;
  motor_all_off();
}

static void compute_pattern_from_packet(const hazard_packet_t *pkt)
{
  // FIXED: conf_q8 (not confidence_q8)
  if (pkt->class_id == 0 || pkt->conf_q8 < kMinConfidenceQ8) {
    apply_pattern_off();
    return;
  }

  float intensity = q8_to_float(pkt->urgency_q8);
  intensity *= q8_to_float(pkt->conf_q8);  // FIXED: conf_q8

  // FIXED: flags bitmask (not turning_fast field)
  if (pkt->flags & FLAG_TURNING_FAST) {
    intensity *= 0.6f;
  }

  if (intensity > 0.0f && intensity < 0.20f) intensity = 0.20f;
  intensity = clamp01(intensity);

  const float    u         = q8_to_float(pkt->urgency_q8);
  const uint32_t period_ms = (uint32_t)(800.0f - (650.0f * u));
  const uint32_t on_ms     = (uint32_t)(0.35f * (float)period_ms);

  g_state.intensity = intensity;
  g_state.period_ms = period_ms;
  g_state.on_ms     = on_ms;

  if (is_dangerous(pkt)) {
    g_state.pattern   = PATTERN_PULSE_BOTH;
    g_state.period_ms = 180;
    g_state.on_ms     = 120;
    return;
  }

  g_state.pattern       = PATTERN_PULSE_ONE;
  g_state.primary_motor = pick_primary_motor(pkt);

#if !HAPTICS_USE_CENTER_MOTOR
  if (pkt->dir == 0) g_state.pattern = PATTERN_PULSE_BOTH;
#endif
}

void haptics_on_packet(const hazard_packet_t *pkt)
{
  if (!pkt) return;
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
  if (on) motor_set_intensity(g_state.primary_motor, g_state.intensity);
}

static void drive_both_motors(bool on)
{
  motor_all_off();
  if (!on) return;
  motor_set_intensity(0, g_state.intensity);
  const uint8_t right = (uint8_t)(motor_driver_num_motors() - 1);
  motor_set_intensity(right, g_state.intensity);
#if HAPTICS_USE_CENTER_MOTOR
  if (g_state.pattern == PATTERN_PULSE_BOTH || g_state.pattern == PATTERN_CONTINUOUS_BOTH) {
    motor_set_intensity(1, 0.7f * g_state.intensity);
  }
#endif
}

static void haptics_task(void *arg)
{
  (void)arg;
  ESP_LOGI(TAG, "Haptics task started");

  drive_both_motors(true);
  vTaskDelay(pdMS_TO_TICKS(80));
  motor_all_off();
  vTaskDelay(pdMS_TO_TICKS(120));

  uint32_t phase_ms = 0;
  while (1) {
    const uint32_t t = now_ms();

    if (g_state.last_packet_ms != 0 && (t - g_state.last_packet_ms) > kPacketTimeoutMs) {
      apply_pattern_off();
    }

    switch (g_state.pattern) {
      case PATTERN_OFF:
        motor_all_off();
        break;
      case PATTERN_PULSE_ONE:
        drive_one_motor(phase_ms < g_state.on_ms);
        break;
      case PATTERN_PULSE_BOTH:
        drive_both_motors(phase_ms < g_state.on_ms);
        break;
      case PATTERN_CONTINUOUS_BOTH:
        drive_both_motors(true);
        break;
      default:
        motor_all_off();
        break;
    }

    phase_ms += kHapticsTickMs;
    if (g_state.period_ms > 0 && phase_ms >= g_state.period_ms) phase_ms = 0;

    vTaskDelay(pdMS_TO_TICKS(kHapticsTickMs));
  }
}

void haptics_start_task(void)
{
  if (g_task != NULL) return;
  memset(&g_state, 0, sizeof(g_state));
  apply_pattern_off();
  xTaskCreate(haptics_task, "haptics", 4096, NULL, 5, &g_task);
}
