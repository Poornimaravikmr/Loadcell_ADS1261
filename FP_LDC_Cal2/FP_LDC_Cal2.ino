#include <SPI.h>

#define CS_PIN     10
#define DRDY_PIN   2
#define RESET_PIN  3

SPISettings spiSettings(5000000, MSBFIRST, SPI_MODE1); // 1 MHz

float gainS = 1.0 / 12953.0685; //put calibration factor in denominator
float fval   = 0.0;
float LCoff = 0.0;

int j =0;

void spiBegin() { digitalWrite(CS_PIN, LOW);}
void spiEnd()   { digitalWrite(CS_PIN, HIGH);}

bool waitForDRDY(unsigned long timeout_ms) {
  unsigned long start = millis();
  while (digitalRead(DRDY_PIN) == HIGH) {
    if ((millis() - start) > timeout_ms) return false;
  }
  return true;
}

uint8_t readRegister(uint8_t reg) {
 uint8_t val;
 spiBegin();
 SPI.transfer(0x20 | reg);
 SPI.transfer(0x00);       
 val = SPI.transfer(0x00);
 spiEnd();
 return val;
}

void writeRegister(uint8_t reg, uint8_t val) {
  spiBegin();
  SPI.transfer(0x40 | reg);
  SPI.transfer(val);
  spiEnd();
}

int32_t readADCDataOnce() {
  spiBegin();
  SPI.transfer(0x12);
  uint8_t Echo = SPI.transfer(0x00);  
  uint8_t b2     = SPI.transfer(0x00);  
  uint8_t b1     = SPI.transfer(0x00);  
  uint8_t b0     = SPI.transfer(0x00); 
  spiEnd();

  int32_t val = ((int32_t)b2 << 16) | ((int32_t)b1 << 8) | (int32_t)b0;
  if (val & 0x800000) {
    val |= 0xFF000000;
  }
  return val;
}

bool getFreshSample(int32_t &adc_counts, uint8_t &status_byte) {
  if (!waitForDRDY(1)) {
    return false;
  }
  adc_counts = readADCDataOnce();
  return true;
}

float GetLCoff() {
  uint8_t dummy_status;
  int32_t tmp;
  for (int i = 0; i <10; i++) {
    getFreshSample(tmp, dummy_status); // ignore result
  }
  long long acc = 0;
  int good = 0;
  for (int i = 0; i <2400; i++) {
    if (getFreshSample(tmp, dummy_status)) {
      acc += tmp;
      good++;
    } else {
      break;
    }
  }
  if (good == 0) return 0.0f;
  float avgCounts = (float)acc / (float)good;
  return avgCounts;
}

void setup() {
  Serial.begin(115200);
  SPI.begin();

  pinMode(CS_PIN, OUTPUT);
  pinMode(DRDY_PIN, INPUT);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);

  digitalWrite(RESET_PIN, LOW);
  digitalWrite(RESET_PIN, HIGH);

  SPI.beginTransaction(spiSettings);
  
  writeRegister(0x02, 0x48); // SINC3 filter, 1200 SPS
  writeRegister(0x06, 0x10); // Internal Reference
  writeRegister(0x10, 0x05); // gain = 32
  writeRegister(0x11, 0x56); // INPMUX: AIN+ - AIN- (Ch1-1,2; Ch2-3,4; Ch3-5,6; Ch4-7,8)


  LCoff = GetLCoff();
  spiBegin();  SPI.transfer(0x08);  spiEnd();
}

void loop() {
  uint8_t status_now = 0;
  int32_t adc_now    = 0;

  if (!getFreshSample(adc_now, status_now)) {
    Serial.println("DRDY timeout");
    return; 
  }

  //  if (j<2400) {
  //   Serial.println(adc_now);
  //   } 
  //   j = j+1;
  // if (j == 2401){
  //     Serial.println("-------");
  //     delay (1000);
  //     j =0;
  //    }

 fval = (float(adc_now)- LCoff) * gainS;
 Serial.println(fval);    

//  delay(100);   
}
