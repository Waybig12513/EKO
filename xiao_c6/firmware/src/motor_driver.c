/*
  motor_driver.c

  Uses ESP-IDF LEDC PWM peripheral to drive 2–3 vibration motors.

  Hardware expectations:
    - Each motor is powered from a supply rail (3V preferred for 3V motors)
    - Each motor is switched with a low-side N-MOSFET or NPN transistor
    - This firmware only outputs PWM on the gate/base control pins
*/

#include <math.h>

#include "esp_check.h"
#include "esp_log.h"

#include "config.h"
#include "haptics/motor_driver.h"

static const char *TAG = "motor_driver";

// LEDC channels we assign to motors.
// Low-speed mode is generally fine for haptics.
static const ledc_mode_t kLedcMode = LEDC_LOW_SPEED_MODE;
static const ledc_timer_t kLedcTimer = LEDC_TIMER_0;

// We keep the mapping in arrays so adding/removing motors is easy.
static const gpio_num_t kMotorPins[] = {
  HAPTIC_MOTOR_LEFT_GPIO,
#if HAPTICS_USE_CENTER_MOTOR
  HAPTIC_MOTOR_CENTER_GPIO,
#endif
  HAPTIC_MOTOR_RIGHT_GPIO,
};

static const ledc_channel_t kMotorChannels[] = {
  LEDC_CHANNEL_0,
#if HAPTICS_USE_CENTER_MOTOR
  LEDC_CHANNEL_1,
#endif
  LEDC_CHANNEL_2,
};

uint8_t motor_driver_num_motors(void)
{
  return (uint8_t)(sizeof(kMotorPins) / sizeof(kMotorPins[0]));
}

void motor_driver_init(void)
{
  // 1) Configure one PWM timer shared by all motor channels.
  ledc_timer_config_t timer_cfg = {
    .speed_mode = kLedcMode,
    .timer_num = kLedcTimer,
    .duty_resolution = HAPTIC_PWM_RESOLUTION,
    .freq_hz = HAPTIC_PWM_FREQUENCY_HZ,
    .clk_cfg = LEDC_AUTO_CLK,
  };

  ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

  // 2) Configure one LEDC channel per motor.
  for (uint8_t i = 0; i < motor_driver_num_motors(); i++) {
    ledc_channel_config_t ch_cfg = {
      .gpio_num = kMotorPins[i],
      .speed_mode = kLedcMode,
      .channel = kMotorChannels[i],
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = kLedcTimer,
      .duty = 0,
      .hpoint = 0,
      .flags.output_invert = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
  }

  ESP_LOGI(TAG, "Motor PWM init: %u motors, %d Hz", motor_driver_num_motors(), HAPTIC_PWM_FREQUENCY_HZ);
}

static uint32_t duty_from_intensity(float intensity_0_to_1)
{
  // Clamp and convert a user-friendly [0..1] float into LEDC duty ticks.
  if (isnan(intensity_0_to_1)) {
    intensity_0_to_1 = 0.0f;
  }
  if (intensity_0_to_1 < 0.0f) intensity_0_to_1 = 0.0f;
  if (intensity_0_to_1 > 1.0f) intensity_0_to_1 = 1.0f;

  // ledc_timer_bit_t enum values are the bit-depth (e.g., 13 for LEDC_TIMER_13_BIT).
  const uint32_t max_duty = (1u << (uint32_t)HAPTIC_PWM_RESOLUTION) - 1u;
  return (uint32_t)lrintf(intensity_0_to_1 * (float)max_duty);
}

void motor_set_intensity(uint8_t motor_index, float intensity_0_to_1)
{
  if (motor_index >= motor_driver_num_motors()) {
    return;
  }

  const uint32_t duty = duty_from_intensity(intensity_0_to_1);
  ESP_ERROR_CHECK(ledc_set_duty(kLedcMode, kMotorChannels[motor_index], duty));
  ESP_ERROR_CHECK(ledc_update_duty(kLedcMode, kMotorChannels[motor_index]));
}

void motor_all_off(void)
{
  for (uint8_t i = 0; i < motor_driver_num_motors(); i++) {
    motor_set_intensity(i, 0.0f);
  }
}
