/*
  nicla_glasses_fusion.ino

  End-to-end pipeline on Nicla Vision (Arduino IDE):

    (Optional Camera) + ToF + IMU  ->  Vision output (fake now / model later)
                                     ->  Fusion produces urgency
                                     ->  Send hazard packet to XIAO over UART

  This matches the architecture in your LaTeX report:
    - Vision model provides class + confidence + direction
    - ToF anchors real distance (proximity)
    - IMU prevents false motion cues during head turns
    - XIAO consumes compact packets and maps to haptics
*/

#include <Arduino.h>
#include "config.h"
#include "vision_types.h"

#include "sensors_imu.h"
#include "sensors_tof.h"
#include "camera_capture.h"
#include "vision_inference.h"
#include "fusion_logic.h"
#include "packet_tx.h"

static uint32_t last_send_ms = 0;

void setup() {
  Serial.begin(USB_SERIAL_BAUD);
  while (!Serial) { delay(10); }

  Serial.println("Nicla Vision: Glasses pipeline starting...");

  // 1) IMU
  if (!imu_init()) {
    Serial.println("ERROR: IMU init failed (Arduino_LSM6DSOX).");
  } else {
    Serial.println("IMU OK");
  }

  // 2) ToF
  if (!tof_init()) {
    Serial.println("ERROR: ToF init failed (VL53L1X on Wire1).");
  } else {
    Serial.println("ToF OK");
  }

  // 3) Camera (optional)
  if (USE_CAMERA) {
    if (!camera_init()) {
      Serial.println("WARN: Camera init failed. Continuing without camera frames.");
    } else {
      Serial.println("Camera OK");
    }
  }

  // 4) Vision (fake now / real model later)
  if (!vision_init()) {
    Serial.println("ERROR: vision init failed.");
  } else {
    Serial.println("Vision module OK");
  }

  // 5) UART link to XIAO
  link_init();
  Serial.println("UART link OK");

  last_send_ms = millis();
}

void loop() {
  // ---- Sensor intake ----
  float gyro_dps = 0.0f;
  (void)imu_read_gyro_dps(gyro_dps);  // if it fails, gyro stays 0

  uint16_t tof_mm = 0;
  (void)tof_read_mm(tof_mm);          // if it fails, tof stays 0

  // Optional camera hook (used later when you plug in a real model)
  if (USE_CAMERA) {
    (void)camera_grab_frame();
  }

  // ---- Vision (model output) ----
  vision_result_t v = vision_run(tof_mm, gyro_dps);

  // ---- Fusion -> urgency ----
  uint8_t urgency_q8 = fuse_to_urgency_q8(v, tof_mm, gyro_dps);

  // ---- Send packet to XIAO at fixed rate ----
  uint32_t now = millis();
  if (now - last_send_ms >= SEND_PERIOD_MS) {
    last_send_ms = now;

    link_send_packet(v, tof_mm, gyro_dps, urgency_q8);

    // Debug print: demonstrates “data intake → quantification → packet”
    Serial.print("ToF(mm)="); Serial.print(tof_mm);
    Serial.print(" gyro(dps)="); Serial.print(gyro_dps, 1);
    Serial.print(" vision_valid="); Serial.print(v.valid);
    Serial.print(" class="); Serial.print(v.class_id);
    Serial.print(" conf="); Serial.print(v.conf, 2);
    Serial.print(" dir="); Serial.print(v.dir);
    Serial.print(" urgency_q8="); Serial.println(urgency_q8);
  }

  delay(5);
}
