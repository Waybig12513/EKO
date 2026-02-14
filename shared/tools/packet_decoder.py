"""
packet_decoder.py

Reads raw bytes (file or stdin) and decodes hazard_packet_t messages.
Good for:
  - debugging the UART stream
  - logging packets to CSV
  - live demos (show ToF, gyro, urgency)

Packet layout matches shared/protocol/hazard_packet.h
"""
import struct
import sys

PKT_LEN = 1+1+4+1+1+1+1+2+2+1+1  # 16 bytes

def crc8_xor(data: bytes) -> int:
  c = 0
  for b in data:
    c ^= b
  return c

def decode_one(buf: bytes):
  ver, seq, t_ms, class_id, conf_q8, dir_, urg_q8, tof_mm, gyro_q8, flags, crc = struct.unpack(
    "<BBIBBbBHHBB", buf
  )
  calc = crc8_xor(buf[:-1])
  return {
    "ok": (calc == crc),
    "ver": ver,
    "seq": seq,
    "t_ms": t_ms,
    "class_id": class_id,
    "conf_q8": conf_q8,
    "dir": dir_,
    "urgency_q8": urg_q8,
    "tof_mm": tof_mm,
    "gyro_dps": gyro_q8 / 256.0,
    "flags": flags,
    "crc": crc,
    "crc_calc": calc
  }

def main():
  data = sys.stdin.buffer.read()
  for i in range(0, len(data), PKT_LEN):
    chunk = data[i:i+PKT_LEN]
    if len(chunk) < PKT_LEN:
      break
    print(decode_one(chunk))

if __name__ == "__main__":
  main()
