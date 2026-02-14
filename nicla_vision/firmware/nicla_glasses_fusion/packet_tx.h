#pragma once
#include <Arduino.h>
#include "vision_types.h"

/*
  packet_tx.h
  Builds and sends hazard_packet_t over Serial1.
*/

bool link_init();
void link_send_packet(const vision_result_t &v, uint16_t tof_mm, float gyro_dps, uint8_t urgency_q8);
