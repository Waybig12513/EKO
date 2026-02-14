#pragma once

/*
  config.h

  Central configuration for the XIAO ESP32-C6 “receiver” firmware.

  This board’s main job in your system:
    1) Receive "hazard" packets from the Nicla Vision (or another host)
    2) Convert those packets into vibration patterns (haptics)

  IMPORTANT HARDWARE NOTE (coin vibration motors):
  ----------------------------------------------
  The coin motors you picked are small brushed DC motors (3V, ~85 mA typical).
  You MUST NOT drive them directly from an ESP32 GPIO.

  Recommended wiring per motor (low-side switch):
    Motor+  -> 3.0V (or 3.3V if acceptable)
    Motor-  -> MOSFET drain (or NPN collector)
    MOSFET source -> GND
    GPIO -> MOSFET gate via ~100Ω (or NPN base via resistor)
    Flyback diode across the motor (cathode to Motor+, anode to Motor-)

  If you skip the transistor/MOSFET, you risk brownouts and/or GPIO damage.
*/

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"

// -------------------------
// Serial link (Nicla -> XIAO)
// -------------------------

// Baud rate used on BOTH sides of the UART link.
#define NICLA_UART_BAUD 115200

// UART instance used for the Nicla link.
#define NICLA_UART_NUM UART_NUM_1

/*
  Default UART pins for XIAO ESP32-C6.
  These correspond to the board's commonly documented RX/TX pins.

  If you wired different pins, change these two lines.
*/
#define NICLA_UART_TX_GPIO GPIO_NUM_21
#define NICLA_UART_RX_GPIO GPIO_NUM_20

// Size of the UART RX buffer used by the driver
#define NICLA_UART_RX_BUFFER_BYTES 1024

// -------------------------
// Haptics (vibration motors)
// -------------------------

/*
  Pick 1–3 output pins, depending on how many motors you physically mount.

  Common layouts:
    - 2 motors: LEFT + RIGHT
    - 3 motors: LEFT + CENTER + RIGHT

  The defaults below are general-purpose GPIOs on ESP32-C6.
  Adjust to your actual wiring.
*/

#define HAPTIC_MOTOR_LEFT_GPIO   GPIO_NUM_2
#define HAPTIC_MOTOR_CENTER_GPIO GPIO_NUM_10
#define HAPTIC_MOTOR_RIGHT_GPIO  GPIO_NUM_3

// Set to 0 if you only wired LEFT/RIGHT and are not using CENTER.
#define HAPTICS_USE_CENTER_MOTOR 1

/*
  PWM settings:
    - Frequency: 150–300 Hz typically feels good for coin motors
    - Resolution: 10–13 bit is plenty
*/
#define HAPTIC_PWM_FREQUENCY_HZ 200

// 13-bit gives duty range 0..8191
#define HAPTIC_PWM_RESOLUTION   LEDC_TIMER_13_BIT
