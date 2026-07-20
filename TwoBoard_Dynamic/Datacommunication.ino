void writeSensorStream() {
  outPayload.newPacket();
  outPayload.add(COPx_B1.fval); outPayload.add(COPy_B1.fval); outPayload.add(WB1); 
  outPayload.add(COPx_B2.fval); outPayload.add(COPy_B2.fval); outPayload.add(WB2); 
  outPayload.add(COPx_B3.fval); outPayload.add(COPy_B3.fval); outPayload.add(WB3); 
  outPayload.add((float)packetCounter); outPayload.add(Timestamp);

  const uint8_t payload_bytes = (uint8_t)(outPayload.sz() * 4);  // 8 floats * 4 = 32 bytes

  // ---- Header: 0xFF 0xFF, len = payload_bytes + 1 (for checksum) ----
  const uint8_t sync0 = 0xFF, sync1 = 0xFF;
  const uint8_t len   = payload_bytes + 1;

  uint8_t chksum = 0xFE;
  chksum += len;
  // (sum all payload bytes)
  for (uint16_t i = 0; i < payload_bytes; ++i) {
    chksum += outPayload.getByte((uint8_t)i);
  }
  
  // const uint16_t total = 3 + payload_bytes + 1;
  static uint8_t buf[3 + 64*4 + 1]; // plenty of headroom (max 64 floats)

  uint16_t p = 0;
  buf[p++] = sync0;
  buf[p++] = sync1;
  buf[p++] = len;

  // payload:
  for (uint16_t i = 0; i < payload_bytes; ++i) {
    buf[p++] = outPayload.getByte((uint8_t)i);
  }

  // checksum:
  buf[p++] = chksum;

  // one fast write:
  Serial.write(buf, p);
}
