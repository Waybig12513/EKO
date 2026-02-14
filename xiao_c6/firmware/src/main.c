/*
  main.c (XIAO ESP32-C6)

  Role in the system
  ------------------
  This firmware is the "receiver + haptics" half of the project:
    - Nicla Vision does sensing + ML inference (camera/ToF/IMU)
    - Nicla sends compact hazard packets over UART
    - XIAO receives packets and turns them into vibration feedback

  What you get out of this skeleton
  --------------------------------
  1) Reliable UART bring-up (pins, baud, driver install)
  2) Packet validation (version + CRC)
  3) A dedicated haptics task that converts packets -> vibration patterns

  Notes
  -----
  - The coin vibration motors MUST be driven through a MOSFET/transistor.
    See config.h for the recommended wiring.
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

// How many bytes we expect per packet. The protocol is a fixed-size struct.
static const size_t kPacketSize = sizeof(hazard_packet_t);

static uint8_t crc8_for_packet(const hazard_packet_t *pkt)
{
  // CRC covers all bytes EXCEPT the last crc8 field itself.
  return crc8_xor((const uint8_t *)pkt, (uint8_t)(sizeof(hazard_packet_t) - 1));
}

static bool hazard_packet_is_valid(const hazard_packet_t *pkt)
{
  if (!pkt) {
    return false;
  }
  if (pkt->ver != HAZARD_PACKET_VER) {
    return false;
  }
  return (crc8_for_packet(pkt) == pkt->crc8);
}

static void nicla_uart_init(void)
{
  uart_config_t uart_config = {
    .baud_rate = NICLA_UART_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(uart_driver_install(
    NICLA_UART_NUM,
    NICLA_UART_RX_BUFFER_BYTES,
    0,                 // no TX buffer
    0,                 // no event queue
    NULL,
    0
  ));

  ESP_ERROR_CHECK(uart_param_config(NICLA_UART_NUM, &uart_config));

  // Explicitly set the UART pins so we're not relying on board defaults.
  ESP_ERROR_CHECK(uart_set_pin(
    NICLA_UART_NUM,
    NICLA_UART_TX_GPIO,
    NICLA_UART_RX_GPIO,
    UART_PIN_NO_CHANGE,
    UART_PIN_NO_CHANGE
  ));

  ESP_LOGI(TAG, "UART init: UART%d, %d baud, TX=%d RX=%d",
           (int)NICLA_UART_NUM, NICLA_UART_BAUD,
           (int)NICLA_UART_TX_GPIO, (int)NICLA_UART_RX_GPIO);
}

static void nicla_rx_task(void *arg)
{
  (void)arg;
  ESP_LOGI(TAG, "RX task started (packet size=%u)", (unsigned)kPacketSize);

  // We read raw bytes, then interpret them as a hazard_packet_t.
  // This assumes the sender writes the struct as a fixed-length binary blob.
  // If you later add framing (sync bytes), this is the place to update.
  uint8_t rx_buf[sizeof(hazard_packet_t)];

  while (1) {
    const int got = uart_read_bytes(NICLA_UART_NUM, rx_buf, kPacketSize, pdMS_TO_TICKS(200));
    if (got != (int)kPacketSize) {
      // Timeout or partial read: ignore and keep waiting.
      continue;
    }

    hazard_packet_t pkt;
    memcpy(&pkt, rx_buf, sizeof(pkt));

    if (!hazard_packet_is_valid(&pkt)) {
      ESP_LOGW(TAG, "Bad packet (ver=%u, crc=%02X expected=%02X)",
               (unsigned)pkt.ver, (unsigned)pkt.crc8, (unsigned)crc8_for_packet(&pkt));
      continue;
    }

    // Log the decoded contents (useful during bring-up).
    ESP_LOGI(TAG,
      "hazard: class=%u dir=%u urg=%u conf=%u turn=%u tof_mm=%u",
      (unsigned)pkt.class_id,
      (unsigned)pkt.dir,
      (unsigned)pkt.urgency_q8,
      (unsigned)pkt.confidence_q8,
      (unsigned)pkt.turning_fast,
      (unsigned)pkt.tof_mm
    );

    // Push packet into haptics policy.
    haptics_on_packet(&pkt);
  }
}

void app_main(void)
{
  ESP_LOGI(TAG, "XIAO ESP32-C6 receiver starting");

  // 1) Init PWM + motors first so we can run a quick self-test.
  motor_driver_init();
  haptics_start_task();

  // 2) Bring up UART link to the Nicla Vision.
  nicla_uart_init();

  // 3) Start RX task to continuously read packets.
  xTaskCreate(nicla_rx_task, "nicla_rx", 4096, NULL, 6, NULL);
}
