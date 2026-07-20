#include <SPI.h>
 
// Teensy SPI0: 11=MOSI (DIN), 12=MISO (DOUT), 13=SCLK, 10=CS
#define CS_PIN     0
#define DRDY_PIN   4
#define RESET_PIN  5
 
SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE1);  // 1 MHz
#define DELAY_US 50
#define DEBUG_RAW_BYTES true
 
void safeDelay() { delayMicroseconds(DELAY_US); }
 
void spiBegin()   { digitalWrite(CS_PIN, LOW);  safeDelay(); }
void spiEnd()     { safeDelay(); digitalWrite(CS_PIN, HIGH); safeDelay(); }
 
void unlockRegisters() {
 spiBegin();
 SPI.transfer(0xF5);
 SPI.transfer(0x00);
 SPI.transfer(0x00);
 spiEnd();
}
 
void writeRegister(uint8_t reg, uint8_t val) {
  spiBegin();
  SPI.transfer(0x40 | reg);   // Write command
//  SPI.transfer(0x00);         // 1-byte only
  SPI.transfer(val);
  spiEnd();
}
 
uint8_t readRegister(uint8_t reg) {
  spiBegin();
  SPI.transfer(0x20 | reg);   // Read command
  SPI.transfer(0x00);         
  uint8_t val = SPI.transfer(0x00);
  spiEnd();
  return val;
}
 
bool waitForDRDY(unsigned long timeout_ms = 1000) {
  unsigned long start = millis();
  while (digitalRead(DRDY_PIN) == HIGH) {
    if ((millis() - start) > timeout_ms) return false;
    delayMicroseconds(10);
  }
  return true;
}
 
int32_t readADCData(bool &statusOK) {
  spiBegin();
  SPI.transfer(0x12);  // RDATA
  uint8_t status = SPI.transfer(0x00);
  uint8_t data[3];
  for (int i = 0; i < 3; i++) data[i] = SPI.transfer(0x00);
  spiEnd();
 
  if (DEBUG_RAW_BYTES) {
    Serial.print("STATUS=0x"); Serial.print(status, HEX);
    Serial.print(" | Bytes: ");
    for (int i = 0; i < 3; i++) {
      Serial.print("0x"); Serial.print(data[i], HEX); Serial.print(" ");
    }
    Serial.println();
  }
 
  statusOK = !(status & 0x80);
  int32_t val = ((int32_t)data[0] << 16) |
                ((int32_t)data[1] << 8) |
                ((int32_t)data[2]);
  if (val & 0x800000) val |= 0xFF000000;
  return val;
}
 
void setupADS1261() {
  writeRegister(0x02, 0x83); 
  writeRegister(0x03, 0x01);  
  writeRegister(0x05, 0x00);  
  writeRegister(0x06, 0x00); 
  writeRegister(0x10, 0x06);  
  writeRegister(0x11, 0x34);  
}
 
void setup() {
  Serial.begin(115200);
  delay(100);
 
  pinMode(CS_PIN, OUTPUT);
  pinMode(DRDY_PIN, INPUT);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);
 
  SPI.begin();
 
  // Hardware reset
  digitalWrite(RESET_PIN, LOW);
  delay(5);
  digitalWrite(RESET_PIN, HIGH);
  delay(50);
 
  SPI.beginTransaction(spiSettings);
  unlockRegisters();
  uint8_t id = readRegister(0x00);
  SPI.endTransaction();
 
  Serial.print("Device ID: 0x"); Serial.println(id, HEX);
 
  if ((id & 0xF0) != 0x80) {
    Serial.println("ADS1261 not found");
  } else {
    Serial.println("ADS1261 found");
  }
 
 setupADS1261();
}
 
void loop() {
 
  Serial.print("Register readback = 0x");
  Serial.print(readRegister(0x02), HEX); Serial.print(",");
  Serial.print(readRegister(0x03), HEX); Serial.print(",");
  Serial.print(readRegister(0x05), HEX); Serial.print(",");
  Serial.print(readRegister(0x06), HEX); Serial.print(",");
  Serial.print(readRegister(0x10), HEX); Serial.print(",");
  Serial.println(readRegister(0x11), HEX);
 
  
 if (!waitForDRDY(1000)) {
   Serial.println("DRDY timeout (no conversion ready)");
   delay(500);
   return;
 }

//  bool ok = false;
//  int32_t adc_raw = readADCData(ok);
//  if (ok) {
//    Serial.print("ADC Raw = "); Serial.println(adc_raw);
//  } else {
//    Serial.println("ADC status error.");
//  }
//
  delay(500);
}