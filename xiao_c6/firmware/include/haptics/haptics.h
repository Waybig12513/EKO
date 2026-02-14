#pragma once

/*
  haptics.h

  Converts high-level "hazard" packets into vibration patterns.

  Design goal:
    - Keep the packet decode task simple: just pass packets to haptics_on_packet()
    - Run actual motor timing/pulsing in a dedicated FreeRTOS task

  This keeps UART parsing responsive and prevents blocking issues.
*/

#include <stdbool.h>
#include <stdint.h>

#include "protocol/hazard_packet.h"

// Start the background haptics task. Call once after motor_driver_init().
void haptics_start_task(void);

// Feed the latest hazard packet into the haptics system.
// The haptics task will adapt the vibration pattern.
void haptics_on_packet(const hazard_packet_t *pkt);

// Force stop (no vibration), regardless of last packet.
void haptics_stop(void);
