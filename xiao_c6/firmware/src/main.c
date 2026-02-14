/*
  main.c (ESP-IDF) - XIAO ESP32-C6 skeleton

  Responsibilities:
    - Read hazard_packet_t from UART
    - Validate CRC
    - Map dir + urgency to haptics (to be added)

  This is a placeholder that matches the Nicla packet format exactly.
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#include "../include/config.h"
#include "../include/protocol/hazard_packet.h"
#include "../include/protocol/crc8.h"

#define UART_PORT UART_NUM_1

static void uart_init(void) {
  uart_config_t cfg = {
    .baud_rate = UART_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };

  uart_driver_install(UART_PORT, 2048, 0, 0, NULL, 0);
  uart_param_config(UART_PORT, &cfg);

  // TODO: set your actual RX/TX pins here once wired:
  // uart_set_pin(UART_PORT, TX_GPIO, RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void app_main(void) {
  uart_init();

  hazard_packet_t p;
  while (1) {
    int r = uart_read_bytes(UART_PORT, (uint8_t*)&p, sizeof(p), pdMS_TO_TICKS(200));
    if (r == sizeof(p)) {
      uint8_t calc = crc8_xor((const uint8_t*)&p, sizeof(p) - 1);
      if (calc == p.crc8 && p.ver == HAZARD_PKT_VERSION) {
        // TODO: haptics mapping: p.dir + p.urgency_q8 -> motor patterns
      } else {
        // TODO: fail safe (motors off)
      }
    } else {
      // TODO: fail safe (motors off)
    }
  }
}
