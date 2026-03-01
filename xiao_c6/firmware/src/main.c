/*
  main.c (XIAO ESP32-C6)

  Role: Receive hazard packets from Nicla Vision over UART,
        validate them, and pass to haptics system.

  Fixes applied:
    - HAZARD_PACKET_VER -> HAZARD_PKT_VERSION
    - pkt->confidence_q8 -> pkt->conf_q8
    - pkt->turning_fast  -> (pkt->flags & FLAG_TURNING_FAST)
    - UART pins corrected to GPIO16(TX)/GPIO17(RX)
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "config.h"
#include "protocol/hazard_packet.h"
#include "protocol/crc8.h"
#include "haptics/motor_driver.h"
#include "haptics/haptics.h"

static const char *TAG = "xiao_c6";
static const size_t kPacketSize = sizeof(hazard_packet_t);

static uint8_t crc8_for_packet(const hazard_packet_t *pkt)
{
  return crc8_xor((const uint8_t *)pkt, (uint8_t)(sizeof(hazard_packet_t) - 1));
}

static bool hazard_packet_is_valid(const hazard_packet_t *pkt)
{
  if (!pkt) return false;
  if (pkt->ver != HAZARD_PKT_VERSION) return false;  // FIXED: was HAZARD_PACKET_VER
  return (crc8_for_packet(pkt) == pkt->crc8);
}

static void nicla_uart_init(void)
{
  uart_config_t uart_config = {
    .baud_rate  = NICLA_UART_BAUD,
    .data_bits  = UART_DATA_8_BITS,
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(uart_driver_install(
    NICLA_UART_NUM,
    NICLA_UART_RX_BUFFER_BYTES,
    0, 0, NULL, 0
  ));
  ESP_ERROR_CHECK(uart_param_config(NICLA_UART_NUM, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(
    NICLA_UART_NUM,
    NICLA_UART_TX_GPIO,
    NICLA_UART_RX_GPIO,
    UART_PIN_NO_CHANGE,
    UART_PIN_NO_CHANGE
  ));

  ESP_LOGI(TAG, "UART init: UART%d, %d baud, TX=GPIO%d RX=GPIO%d",
           (int)NICLA_UART_NUM, NICLA_UART_BAUD,
           (int)NICLA_UART_TX_GPIO, (int)NICLA_UART_RX_GPIO);
}

static void nicla_rx_task(void *arg)
{
  (void)arg;
  ESP_LOGI(TAG, "RX task started (packet size=%u)", (unsigned)kPacketSize);

  uint8_t rx_buf[sizeof(hazard_packet_t)];

  while (1) {
    const int got = uart_read_bytes(NICLA_UART_NUM, rx_buf, kPacketSize, pdMS_TO_TICKS(200));
    if (got != (int)kPacketSize) continue;

    hazard_packet_t pkt;
    memcpy(&pkt, rx_buf, sizeof(pkt));

    if (!hazard_packet_is_valid(&pkt)) {
      ESP_LOGW(TAG, "Bad packet (ver=%u, crc=%02X expected=%02X)",
               (unsigned)pkt.ver, (unsigned)pkt.crc8, (unsigned)crc8_for_packet(&pkt));
      continue;
    }

    // FIXED: use conf_q8 (not confidence_q8) and flags bitmask (not turning_fast)
    ESP_LOGI(TAG,
      "hazard: class=%u dir=%d urg=%u conf=%u turning=%u tof_mm=%u",
      (unsigned)pkt.class_id,
      (int)pkt.dir,
      (unsigned)pkt.urgency_q8,
      (unsigned)pkt.conf_q8,
      (unsigned)((pkt.flags & FLAG_TURNING_FAST) ? 1 : 0),
      (unsigned)pkt.tof_mm
    );

    haptics_on_packet(&pkt);
  }
}

void app_main(void)
{
  ESP_LOGI(TAG, "XIAO ESP32-C6 receiver starting");
  motor_driver_init();
  haptics_start_task();
  nicla_uart_init();
  xTaskCreate(nicla_rx_task, "nicla_rx", 4096, NULL, 6, NULL);
}
