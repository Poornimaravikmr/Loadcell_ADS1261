#include "Arduino.h"
#include "Variables.h"
#include "CustomDS.h"
#include <SPI.h>

#define CS0_PIN     10
#define DRDY0_PIN   2
#define RESET0_PIN  3

#define CS1_PIN     0
#define DRDY1_PIN   4             
#define RESET1_PIN  5

#define CS2_PIN     44
#define DRDY2_PIN   6             
#define RESET2_PIN  7

SPISettings spi0Settings(5000000, MSBFIRST, SPI_MODE1);
SPISettings spi1Settings(5000000, MSBFIRST, SPI_MODE1);
SPISettings spi2Settings(5000000, MSBFIRST, SPI_MODE1);

static inline void spi0Select()   { digitalWriteFast(CS0_PIN, LOW);  }
static inline void spi0Deselect() { digitalWriteFast(CS0_PIN, HIGH); }

static inline void spi1Select()   { digitalWriteFast(CS1_PIN, LOW);  }
static inline void spi1Deselect() { digitalWriteFast(CS1_PIN, HIGH); }

static inline void spi2Select()   { digitalWriteFast(CS2_PIN, LOW);  }
static inline void spi2Deselect() { digitalWriteFast(CS2_PIN, HIGH); }

static inline void spi0StartTxn() { SPI.beginTransaction(spi0Settings); }
static inline void spi0EndTxn()   { SPI.endTransaction(); }

static inline void spi1StartTxn() { SPI.beginTransaction(spi1Settings); }
static inline void spi1EndTxn()   { SPI.endTransaction(); }

static inline void spi2StartTxn() { SPI.beginTransaction(spi2Settings); }
static inline void spi2EndTxn()   { SPI.endTransaction(); }

//===========================================================================================

static inline void writeRegister0_fast(uint8_t reg, uint8_t val) {
  spi0Select();
  SPI.transfer((uint8_t)(0x40 | reg));
  SPI.transfer(val);
  spi0Deselect();
}

static inline void writeRegister1_fast(uint8_t reg, uint8_t val) {
  spi1Select();
  SPI.transfer((uint8_t)(0x40 | reg));
  SPI.transfer(val);
  spi1Deselect();
}

static inline void writeRegister2_fast(uint8_t reg, uint8_t val) {
  spi2Select();
  SPI.transfer((uint8_t)(0x40 | reg));
  SPI.transfer(val);
  spi2Deselect();
}

static inline void selectDiff0(uint8_t ainp, uint8_t ainn) {
  uint8_t mux = (uint8_t)(((ainp & 0x0F) << 4) | (ainn & 0x0F));
  writeRegister0_fast(0x11, mux);
}

static inline void selectDiff1(uint8_t ainp, uint8_t ainn) {
  uint8_t mux = (uint8_t)(((ainp & 0x0F) << 4) | (ainn & 0x0F));
  writeRegister1_fast(0x11, mux);
}

static inline void selectDiff2(uint8_t ainp, uint8_t ainn) {
  uint8_t mux = (uint8_t)(((ainp & 0x0F) << 4) | (ainn & 0x0F));
  writeRegister2_fast(0x11, mux);
}
// ============================================================================================

static inline void ads0_start() { spi0Select(); SPI.transfer((uint8_t)0x08); spi0Deselect(); }
static inline void ads0_stop()  { spi0Select(); SPI.transfer((uint8_t)0x0A); spi0Deselect(); }

static inline void ads1_start() { spi1Select(); SPI.transfer((uint8_t)0x08); spi1Deselect(); }
static inline void ads1_stop()  { spi1Select(); SPI.transfer((uint8_t)0x0A); spi1Deselect(); }

static inline void ads2_start() { spi2Select(); SPI.transfer((uint8_t)0x08); spi2Deselect(); }
static inline void ads2_stop()  { spi2Select(); SPI.transfer((uint8_t)0x0A); spi2Deselect(); }

// ============================================================================================

static inline void ConfigADS0() {
  writeRegister0_fast(0x02, 0x83);  // DOR 40kSPS
  writeRegister0_fast(0x03, 0x01);  // set to 21 for CHOP mode   
  writeRegister0_fast(0x05, 0x00);  // set to 40 for status byte
  writeRegister0_fast(0x06, 0x10);  // Internal Reference
  writeRegister0_fast(0x10, 0x05);  // Gain 32
}

static inline void ConfigADS1() {
  writeRegister1_fast(0x02, 0x83);  // DOR 40kSPS
  writeRegister1_fast(0x03, 0x01);  // set to 21 for CHOP mode   
  writeRegister1_fast(0x05, 0x00);  // set to 40 for status byte
  writeRegister1_fast(0x06, 0x10);  // Internal Reference
  writeRegister1_fast(0x10, 0x05);  // Gain 32
}

static inline void ConfigADS2() {
  writeRegister2_fast(0x02, 0x83);  // DOR 40kSPS
  writeRegister2_fast(0x03, 0x01);  // set to 21 for CHOP mode   
  writeRegister2_fast(0x05, 0x00);  // set to 40 for status byte
  writeRegister2_fast(0x06, 0x10);  // Internal Reference
  writeRegister2_fast(0x10, 0x05);  // Gain 32
}
// ============================================================================================

void SelfOffCal_ADS0() {
  ads0_stop();
  delay(2);
  spi0Select();
  SPI.transfer(0x19);
  spi0Deselect();
  while (digitalRead(DRDY0_PIN) == HIGH) {}
  ads0_start();
}

void SelfOffCal_ADS1() {
  ads1_stop();
  delay(2);
  spi1Select();
  SPI.transfer(0x19);
  spi1Deselect();
  while (digitalRead(DRDY1_PIN) == HIGH) {}
  ads1_start();
}

void SelfOffCal_ADS2() {
  ads2_stop();
  delay(2);
  spi2Select();
  SPI.transfer(0x19);
  spi2Deselect();
  while (digitalRead(DRDY2_PIN) == HIGH) {}
  ads2_start();
}

// ============================================================================================
volatile int32_t  adc_buffer[TOTAL_CHANNELS][BUFFER_SIZE];
volatile uint16_t buffer_write_index = 0;
volatile uint16_t buffer_read_index  = 0;

volatile uint8_t  current_channel_adc0 = 0;
volatile uint8_t  current_channel_adc1 = 0;
volatile uint8_t  current_channel_adc2 = 0;

volatile bool     acq_paused = false;
volatile uint32_t rb_full_events = 0;

volatile bool in_isr0 = false;
volatile bool in_isr1 = false;
volatile bool in_isr2 = false;

volatile uint32_t frames_pending = 0;

volatile bool     offcap0 = false;
volatile uint8_t  offcap_p0 = 0, offcap_n0 = 0;
volatile int64_t  offcap_acc0 = 0;
volatile uint16_t offcap_need0 = 0;
volatile uint16_t offcap_got0  = 0;
volatile bool     offcap_done0 = false;

volatile bool     offcap1 = false;
volatile uint8_t  offcap_p1 = 0, offcap_n1 = 0;
volatile int64_t  offcap_acc1 = 0;
volatile uint16_t offcap_need1 = 0;
volatile uint16_t offcap_got1  = 0;
volatile bool     offcap_done1 = false;

volatile bool     offcap2 = false;
volatile uint8_t  offcap_p2 = 0, offcap_n2 = 0;
volatile int64_t  offcap_acc2 = 0;
volatile uint16_t offcap_need2 = 0;
volatile uint16_t offcap_got2  = 0;
volatile bool     offcap_done2 = false;

static inline bool rbHasSpaceForNextFrame() {
  uint16_t next = (uint16_t)((buffer_write_index + 1) % BUFFER_SIZE);
  return (next != buffer_read_index);
}

static inline void pauseAcquisitionFromISR() {
  acq_paused = true;
  rb_full_events++;
}

// =============================== Conversion Data Reads ====================================

static inline int32_t readConv0_24b_signext() {
  spi0Select();
  SPI.transfer((uint8_t)0x12);
  (void)SPI.transfer((uint8_t)0x00);
  uint8_t b2 = SPI.transfer((uint8_t)0x00);
  uint8_t b1 = SPI.transfer((uint8_t)0x00);
  uint8_t b0 = SPI.transfer((uint8_t)0x00);
  spi0Deselect();

  int32_t raw = ((int32_t)b2 << 16) | ((int32_t)b1 << 8) | (int32_t)b0;
  if (raw & 0x800000) raw |= 0xFF000000;

  // Serial.println(raw);
  return raw;
}

static inline int32_t readConv1_24b_signext() {
  spi1Select();
  SPI.transfer((uint8_t)0x12);
  (void)SPI.transfer((uint8_t)0x00);
  uint8_t b2 = SPI.transfer((uint8_t)0x00);
  uint8_t b1 = SPI.transfer((uint8_t)0x00);
  uint8_t b0 = SPI.transfer((uint8_t)0x00);
  spi1Deselect();

  int32_t raw = ((int32_t)b2 << 16) | ((int32_t)b1 << 8) | (int32_t)b0;
  if (raw & 0x800000) raw |= 0xFF000000;

  // Serial.println(raw);
  return raw;
}

static inline int32_t readConv2_24b_signext() {
  spi2Select();
  SPI.transfer((uint8_t)0x12);
  (void)SPI.transfer((uint8_t)0x00);
  uint8_t b2 = SPI.transfer((uint8_t)0x00);
  uint8_t b1 = SPI.transfer((uint8_t)0x00);
  uint8_t b0 = SPI.transfer((uint8_t)0x00);
  spi2Deselect();

  int32_t raw = ((int32_t)b2 << 16) | ((int32_t)b1 << 8) | (int32_t)b0;
  if (raw & 0x800000) raw |= 0xFF000000;

  // Serial.println(raw);
  return raw;
}

// ===========================================================================================
void DRDY0_ISR() {
  if (acq_paused) return;
  if (in_isr0) return;
  in_isr0 = true;

  if (digitalReadFast(DRDY0_PIN) != LOW) { in_isr0 = false; return; }

  if (!offcap0 && !offcap1 && !offcap2 && current_channel_adc0 == 0 && current_channel_adc1 == 0 && current_channel_adc2 == 0) {
    if (!rbHasSpaceForNextFrame()) { pauseAcquisitionFromISR(); in_isr0 = false; return; }
  }

  int32_t rawCounts = readConv0_24b_signext();

  if (offcap0) {
    offcap_acc0 += (int64_t)rawCounts;
    if (++offcap_got0 >= offcap_need0) offcap_done0 = true;
    in_isr0 = false;
    return;
  }

  adc_buffer[current_channel_adc0][buffer_write_index] = rawCounts;

  current_channel_adc0 = (uint8_t)((current_channel_adc0 + 1) % NUM_CHANNELS_PER_ADC);
  uint8_t next_p = ADC0_MUX_P_PINS[current_channel_adc0];
  uint8_t next_n = ADC0_MUX_N_PINS[current_channel_adc0];
  uint8_t mux    = (uint8_t)(((next_p & 0x0F) << 4) | (next_n & 0x0F));

  spi0Select();
  SPI.transfer((uint8_t)(0x40 | 0x11));
  SPI.transfer(mux);
  spi0Deselect();

  in_isr0 = false;
}

void DRDY1_ISR() {
  if (acq_paused) return;
  if (in_isr1) return;
  in_isr1 = true;

  if (digitalReadFast(DRDY1_PIN) != LOW) { in_isr1 = false; return; }

  if (!offcap0 && !offcap1 && !offcap2 && current_channel_adc0 == 0 && current_channel_adc1 == 0 && current_channel_adc2 == 0) {
    if (!rbHasSpaceForNextFrame()) { pauseAcquisitionFromISR(); in_isr1 = false; return; }
  }
  int32_t rawCounts = readConv1_24b_signext();

  if (offcap1) {
    offcap_acc1 += (int64_t)rawCounts;
    if (++offcap_got1 >= offcap_need1) offcap_done1 = true;
    in_isr1 = false;
    return;
  }

  adc_buffer[current_channel_adc1 + NUM_CHANNELS_PER_ADC][buffer_write_index] = rawCounts;

  current_channel_adc1 = (uint8_t)((current_channel_adc1 + 1) % NUM_CHANNELS_PER_ADC);
  uint8_t next_p = ADC1_MUX_P_PINS[current_channel_adc1];
  uint8_t next_n = ADC1_MUX_N_PINS[current_channel_adc1];
  uint8_t mux    = (uint8_t)(((next_p & 0x0F) << 4) | (next_n & 0x0F));

  spi1Select();
  SPI.transfer((uint8_t)(0x40 | 0x11));
  SPI.transfer(mux);
  spi1Deselect();
  
  if (!offcap0 && !offcap1 && !offcap2 && current_channel_adc0 == 0 && current_channel_adc1 == 0 && current_channel_adc2 == 0) {
    buffer_write_index = (uint16_t)((buffer_write_index + 1) % BUFFER_SIZE);
    frames_pending++;
  }
  
  in_isr1 = false;
}

void DRDY2_ISR() {
  if (acq_paused) return;
  if (in_isr2) return;
  in_isr2 = true;

  if (digitalReadFast(DRDY2_PIN) != LOW) { in_isr2 = false; return; }

  if (!offcap0 && !offcap1 && !offcap2 && current_channel_adc0 == 0 && current_channel_adc1 == 0 && current_channel_adc2 == 0 ) {
    if (!rbHasSpaceForNextFrame()) { pauseAcquisitionFromISR(); in_isr2 = false; return; }
  }
  int32_t rawCounts = readConv2_24b_signext();

  if (offcap2) {
    offcap_acc2 += (int64_t)rawCounts;
    if (++offcap_got2 >= offcap_need2) offcap_done2 = true;
    in_isr2 = false;
    return;
  }

  adc_buffer[current_channel_adc2 + (2 * NUM_CHANNELS_PER_ADC)][buffer_write_index] = rawCounts;

  current_channel_adc2 = (uint8_t)((current_channel_adc2 + 1) % NUM_CHANNELS_PER_ADC);
  uint8_t next_p = ADC2_MUX_P_PINS[current_channel_adc2];
  uint8_t next_n = ADC2_MUX_N_PINS[current_channel_adc2];
  uint8_t mux    = (uint8_t)(((next_p & 0x0F) << 4) | (next_n & 0x0F));

  spi2Select();
  SPI.transfer((uint8_t)(0x40 | 0x11));
  SPI.transfer(mux);
  spi2Deselect();

  if (!offcap0 && !offcap1 && !offcap2 && current_channel_adc0 == 0 && current_channel_adc1 == 0 && current_channel_adc2 == 0) {
    buffer_write_index = (uint16_t)((buffer_write_index + 1) % BUFFER_SIZE);
    frames_pending++;
  }

  in_isr2 = false;
}
// ===================================== Processing ======================================
uint32_t processFrames(uint32_t maxFramesToProcess = 0) {
  uint32_t processed = 0;

  while (buffer_read_index != buffer_write_index) {

    if (maxFramesToProcess && processed >= maxFramesToProcess) break;

    uint16_t local_read_index;
    int32_t rawCounts[TOTAL_CHANNELS];

    noInterrupts();
    local_read_index = buffer_read_index;
    buffer_read_index = (uint16_t)((buffer_read_index + 1) % BUFFER_SIZE);
    if (frames_pending) frames_pending--;
    for (int i = 0; i < TOTAL_CHANNELS; i++) {
      rawCounts[i] = adc_buffer[i][local_read_index];
    }
    interrupts();

      float f_now[12];
      f_now[0] = ((float)rawCounts[0] - Lc1Off) * gainB11;
      f1.fval = f_now[0];
      f_now[1] = ((float)rawCounts[1] - Lc2Off) * gainB12;
      f2.fval = f_now[1];
      f_now[2] = ((float)rawCounts[2] - Lc3Off) * gainB13;
      f3.fval = f_now[2];
      f_now[3] = ((float)rawCounts[3] - Lc4Off) * gainB14;
      f4.fval = f_now[3];
      f_now[4] = ((float)rawCounts[4] - Lc5Off) * gainB21;
      f5.fval = f_now[4];
      f_now[5] = ((float)rawCounts[5] - Lc6Off) * gainB22;
      f6.fval = f_now[5];
      f_now[6] = ((float)rawCounts[6] - Lc7Off) * gainB23;
      f7.fval = f_now[6];
      f_now[7] = ((float)rawCounts[7] - Lc8Off) * gainB24;
      f8.fval = f_now[7];
      f_now[8] = ((float)rawCounts[8] - Lc9Off) * gainB31;
      f9.fval = f_now[8];
      f_now[9] = ((float)rawCounts[9] - Lc10Off) * gainB32;
      f10.fval = f_now[9];
      f_now[10] = ((float)rawCounts[10] - Lc11Off) * gainB33;
      f11.fval = f_now[10];
      f_now[11] = ((float)rawCounts[11] - Lc12Off) * gainB34;
      f12.fval = f_now[11];
      
    UpdateCOP0(f1.fval, f2.fval, f3.fval, f4.fval);
    UpdateCOP1(f5.fval, f6.fval, f7.fval, f8.fval);
    UpdateCOP2(f9.fval, f10.fval, f11.fval, f12.fval);

   Serial.print(COPx_B1.fval); Serial.print('\t');
   Serial.print(COPy_B1.fval); Serial.print('\t');
   Serial.print(WB1); Serial.print('\t');
   Serial.print(COPx_B2.fval); Serial.print('\t');
   Serial.print(COPy_B2.fval); Serial.print('\t');
   Serial.print(WB2); Serial.print('\t');
   Serial.print(COPx_B3.fval); Serial.print('\t');
   Serial.print(COPy_B3.fval); Serial.print('\t');
   Serial.print(WB3); Serial.print('\t');

   Serial.print(f1.fval); Serial.print('\t');
   Serial.print(f2.fval); Serial.print('\t');
   Serial.print(f3.fval); Serial.print('\t');
   Serial.print(f4.fval); Serial.print('\t');
   Serial.print(f5.fval); Serial.print('\t');
   Serial.print(f6.fval); Serial.print('\t');
   Serial.print(f7.fval); Serial.print('\t');
   Serial.print(f8.fval); Serial.print('\t');
   Serial.print(f9.fval); Serial.print('\t');
   Serial.print(f10.fval); Serial.print('\t');
   Serial.print(f11.fval); Serial.print('\t');
   Serial.println(f12.fval); 
    processed++;
  }
  return processed;
}

// =============================== Acquisition Pause/Resume ============================
static inline void stopAcqHard() {
  detachInterrupt(digitalPinToInterrupt(DRDY0_PIN));
  detachInterrupt(digitalPinToInterrupt(DRDY1_PIN));
  detachInterrupt(digitalPinToInterrupt(DRDY2_PIN));
  ads0_stop();
  ads1_stop();
  ads2_stop();
  spi0EndTxn();
  spi1EndTxn();
  spi2EndTxn();
}

static inline void startAcqHard() {
  spi0StartTxn();
  spi1StartTxn();
  spi2StartTxn();

  selectDiff0(ADC0_MUX_P_PINS[0], ADC0_MUX_N_PINS[0]);
  selectDiff1(ADC1_MUX_P_PINS[0], ADC1_MUX_N_PINS[0]);
  selectDiff2(ADC2_MUX_P_PINS[0], ADC2_MUX_N_PINS[0]);

  current_channel_adc0 = 0;
  current_channel_adc1 = 0;
  current_channel_adc2 = 0;

  attachInterrupt(digitalPinToInterrupt(DRDY0_PIN), DRDY0_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(DRDY1_PIN), DRDY1_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(DRDY2_PIN), DRDY2_ISR, FALLING);

  ads0_start();
  ads1_start();
  ads2_start();
}

// =====================================================================================
volatile bool tare_requested = false;
void performTare() {
  stopAcqHard();

  Lc1Off = GetLCoff0(1,2);
  Lc2Off = GetLCoff0(3,4);
  Lc3Off = GetLCoff0(5,6);
  Lc4Off = GetLCoff0(7,8);

  Lc5Off = GetLCoff1(1,2);
  Lc6Off = GetLCoff1(3,4);
  Lc7Off = GetLCoff1(5,6);
  Lc8Off = GetLCoff1(7,8);

  Lc9Off = GetLCoff2(1,2);
  Lc10Off = GetLCoff2(3,4);
  Lc11Off = GetLCoff2(5,6);
  Lc12Off = GetLCoff2(7,8);

  InitSensorVals();

  noInterrupts();
  buffer_write_index    = 0;
  buffer_read_index     = 0;
  frames_pending        = 0;
  acq_paused            = false;
  interrupts();

  startAcqHard();
}

// ======================================================================================
void setup() {
  Serial.begin(115200);

  pinMode(CS0_PIN, OUTPUT);
  pinMode(DRDY0_PIN, INPUT);
  pinMode(RESET0_PIN, OUTPUT);
  digitalWriteFast(CS0_PIN, HIGH);
  digitalWriteFast(RESET0_PIN, HIGH);

  pinMode(CS1_PIN, OUTPUT);
  pinMode(DRDY1_PIN, INPUT);
  pinMode(RESET1_PIN, OUTPUT);
  digitalWriteFast(CS1_PIN, HIGH);
  digitalWriteFast(RESET1_PIN, HIGH);

  pinMode(CS2_PIN, OUTPUT);
  pinMode(DRDY2_PIN, INPUT);
  pinMode(RESET2_PIN, OUTPUT);
  digitalWriteFast(CS2_PIN, HIGH);
  digitalWriteFast(RESET2_PIN, HIGH);

  digitalWriteFast(RESET0_PIN, LOW); delayMicroseconds(10); digitalWriteFast(RESET0_PIN, HIGH);
  digitalWriteFast(RESET1_PIN, LOW); delayMicroseconds(10); digitalWriteFast(RESET1_PIN, HIGH);
  digitalWriteFast(RESET2_PIN, LOW); delayMicroseconds(10); digitalWriteFast(RESET2_PIN, HIGH);
  SPI.begin();

  SPI.usingInterrupt(digitalPinToInterrupt(DRDY0_PIN));
  SPI.usingInterrupt(digitalPinToInterrupt(DRDY1_PIN));
  SPI.usingInterrupt(digitalPinToInterrupt(DRDY2_PIN));

  spi0StartTxn();
  ConfigADS0();
  SelfOffCal_ADS0();

  spi1StartTxn();
  ConfigADS1();
  SelfOffCal_ADS1();

  spi2StartTxn();
  ConfigADS2();
  SelfOffCal_ADS2();

  Lc1Off = GetLCoff0(1,2);
  Lc2Off = GetLCoff0(3,4);
  Lc3Off = GetLCoff0(5,6);
  Lc4Off = GetLCoff0(7,8);

  Lc5Off = GetLCoff1(1,2);
  Lc6Off = GetLCoff1(3,4);
  Lc7Off = GetLCoff1(5,6);
  Lc8Off = GetLCoff1(7,8);

  Lc9Off = GetLCoff2(1,2);
  Lc10Off = GetLCoff2(3,4);
  Lc11Off = GetLCoff2(5,6);
  Lc12Off = GetLCoff2(7,8);

  InitSensorVals();
  startAcqHard();
}

void loop() {
  while (Serial.available() > 0) {
    int b = Serial.read();
    if (b == '1') tare_requested = true;
  }
  if (tare_requested) {
    performTare();
    tare_requested = false;
  }

  if (acq_paused) {
    stopAcqHard();
    processFrames(0);
    noInterrupts();
    acq_paused = false;
    interrupts();
    startAcqHard();
  }

  processFrames(0);

}
