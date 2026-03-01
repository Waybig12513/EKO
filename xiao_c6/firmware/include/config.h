#pragma once

/*
  config.h — XIAO ESP32-C6 firmware configuration

  Fixes applied:
    - UART pins: TX=GPIO16, RX=GPIO17 (matches confirmed wiring)
    - Motor pins: LEFT=GPIO19, RIGHT=GPIO20 (matches confirmed wiring)
    - HAPTICS_USE_CENTER_MOTOR set to 0 (only 2 motors in hardware)
*/

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"

// -------------------------
// UART link (Nicla -> XIAO)
// -------------------------
#define NICLA_UART_BAUD             115200
#define NICLA_UART_NUM              UART_NUM_1
#define NICLA_UART_TX_GPIO          GPIO_NUM_16   // FIXED: D6 pad, confirmed wiring
#define NICLA_UART_RX_GPIO          GPIO_NUM_17   // FIXED: D7 pad, confirmed wiring
#define NICLA_UART_RX_BUFFER_BYTES  1024

// -------------------------
// Haptics (vibration motors)
// -------------------------
// 2-motor layout: LEFT leg + RIGHT leg
// Each motor wired via NPN transistor + 1kΩ base resistor + flyback diode
#define HAPTIC_MOTOR_LEFT_GPIO      GPIO_NUM_19   // FIXED: D8 pad, confirmed wiring
#define HAPTIC_MOTOR_CENTER_GPIO    GPIO_NUM_10   // unused — kept for reference
#define HAPTIC_MOTOR_RIGHT_GPIO     GPIO_NUM_20   // FIXED: D9 pad, confirmed wiring

// FIXED: 0 — only 2 motors physically wired (LEFT + RIGHT, no CENTER)
#define HAPTICS_USE_CENTER_MOTOR    0

// PWM: 200 Hz feels natural for coin vibration motors
#define HAPTIC_PWM_FREQUENCY_HZ     200
#define HAPTIC_PWM_RESOLUTION       LEDC_TIMER_13_BIT
