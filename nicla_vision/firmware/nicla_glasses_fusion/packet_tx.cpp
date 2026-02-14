#include "config.h"
#include "packet_tx.h"

// These are duplicated into the sketch folder for Arduino IDE compatibility.
#include "hazard_packet.h"
#include "crc8.h"

static HardwareSerial &LINK = Serial1;
static uint8_t seq_id = 0;

bool link_init() {
  /*
    Initializes UART link to the XIAO.
  */
  LINK.begin(LINK_SERIAL_BAUD);
  return true;
}

void link_send_packet(const vision_result_t &v, uint16_t tof_mm, float gyro_dps, uint8_t urgency_q8) {
  /*
    Populate packet fields, compute CRC, and send bytes.

    Design choice:
      - Send compact metadata only (no images)
      - Keep packet fixed-size and easy to parse on embedded
  */
  hazard_packet_t p = {0};

  p.ver = HAZARD_PKT_VERSION;
  p.seq = seq_id++;
  p.t_ms = millis();

  p.class_id = v.valid ? v.class_id : 0;
  float conf01 = v.valid ? v.conf : 0.0f;
  if (conf01 < 0) conf01 = 0;
  if (conf01 > 1) conf01 = 1;
  p.conf_q8 = (uint8_t)roundf(conf01 * 255.0f);

  p.dir = v.valid ? v.dir : 0;
  p.urgency_q8 = urgency_q8;

  // 0 means invalid for our ToF layer.
  p.tof_mm = tof_mm;

  // Encode gyro magnitude as (deg/s * 256) to preserve decimals cheaply.
  if (gyro_dps < 0) gyro_dps = 0;
  p.gyro_dps_q8 = (uint16_t)roundf(gyro_dps * 256.0f);

  p.flags = 0;
  if (gyro_dps > TURN_FAST_DPS) p.flags |= FLAG_TURNING_FAST;
  if (tof_mm > 0)              p.flags |= FLAG_TOF_VALID;
  if (v.valid)                 p.flags |= FLAG_VISION_VALID;

  p.crc8 = crc8_xor((const uint8_t*)&p, sizeof(p) - 1);

  LINK.write((const uint8_t*)&p, sizeof(p));
}
