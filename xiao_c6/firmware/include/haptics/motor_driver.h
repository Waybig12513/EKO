#pragma once

/*
  motor_driver.h

  Thin abstraction for controlling 1–3 coin vibration motors using PWM.

  The motors MUST be driven through a transistor/MOSFET stage.
  GPIOs are only used to output a PWM signal to the gate/base.
*/

#include <stdbool.h>
#include <stdint.h>

// How many motors are compiled in (2 or 3 depending on config.h)
uint8_t motor_driver_num_motors(void);

// Initialize LEDC PWM for the configured motor pins.
// Safe to call once at boot.
void motor_driver_init(void);

// Set motor intensity in the range [0.0, 1.0].
//  - 0.0 turns it off
//  - 1.0 is max duty
void motor_set_intensity(uint8_t motor_index, float intensity_0_to_1);

// Immediately stop all motors.
void motor_all_off(void);
