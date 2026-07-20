#include "Arduino.h"
#include "Variable.h"
#include "CustomDS.h"
#include <SPI.h>

#define CS0_PIN     10
#define DRDY0_PIN   2
#define RESET0_PIN  3

SPISettings spi0Settings(5000000, MSBFIRST, SPI_MODE1);

static inline void spi0Select()   { digitalWriteFast(CS0_PIN, LOW);  }
static inline void spi0Deselect() { digitalWriteFast(CS0_PIN, HIGH); }

static inline void spi0StartTxn() { SPI.beginTransaction(spi0Settings); }
static inline void spi0EndTxn()   { SPI.endTransaction(); }

//===========================================================================================
static inline void writeRegister0_fast(uint8_t reg, uint8_t val) {
  spi0Select();
  SPI.transfer((uint8_t)(0x40 | reg));
  SPI.transfer(val);
  spi0Deselect();
}

static inline void selectDiff0(uint8_t ainp, uint8_t ainn) { 
  uint8_t mux = (uint8_t)(((ainp & 0x0F) << 4) | (ainn & 0x0F));
  writeRegister0_fast(0x11, mux);
}
// ============================================================================================
static inline void ads0_start() { spi0Select(); SPI.transfer((uint8_t)0x08); spi0Deselect(); }
static inline void ads0_stop()  { spi0Select(); SPI.transfer((uint8_t)0x0A); spi0Deselect(); }

// ============================================================================================
static inline void ConfigADS0() {
  writeRegister0_fast(0x02, 0x83);  // DOR 40kSPS
  writeRegister0_fast(0x03, 0x01);  // set to 21 for CHOP mode   
  writeRegister0_fast(0x05, 0x00);  // set to 40 for status byte
  writeRegister0_fast(0x06, 0x10);  // Internal Reference
  writeRegister0_fast(0x10, 0x05);  // Gain 32
}

// ============================================================================================
// void SelfOffCal_ADS0() {
//   ads0_stop();
//   delay(2);
//   spi0Select();
//   SPI.transfer(0x19);
//   spi0Deselect();
//   while (digitalRead(DRDY0_PIN) == HIGH) {}
//   ads0_start();
// }
// ============================================================================================
volatile int32_t  adc_buffer[TOTAL_CHANNELS][BUFFER_SIZE];
volatile uint16_t buffer_write_index = 0;
volatile uint16_t buffer_read_index  = 0;

volatile uint8_t  current_channel_adc0 = 0;

volatile bool     acq_paused = false;
volatile uint32_t rb_full_events = 0;

volatile bool in_isr0 = false;

volatile uint32_t frames_pending = 0;

volatile bool     offcap0 = false;
volatile uint8_t  offcap_p0 = 0, offcap_n0 = 0;
volatile int64_t  offcap_acc0 = 0;
volatile uint16_t offcap_need0 = 0;
volatile uint16_t offcap_got0  = 0;
volatile bool     offcap_done0 = false;

// static inline bool rbHasSpaceForNextFrame() {
//   uint16_t next = (uint16_t)((buffer_write_index + 1) % BUFFER_SIZE);
//   return (next != buffer_read_index);
// }

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
// ===========================================================================================
void DRDY0_ISR() {
  if (acq_paused) return;
  if (in_isr0) return;
  in_isr0 = true;

  if (digitalReadFast(DRDY0_PIN) != LOW) { in_isr0 = false; return; }

  // if (!offcap0 && current_channel_adc0 == 0) {
  //   if (!rbHasSpaceForNextFrame()) { pauseAcquisitionFromISR(); in_isr0 = false; return; }
  // }

  int32_t rawCounts = readConv0_24b_signext();

  if (offcap0) {
    offcap_acc0 += (int64_t)rawCounts;
    if (++offcap_got0 >= offcap_need0) offcap_done0 = true;
    in_isr0 = false;
    return;
  }

  adc_buffer[current_channel_adc0][buffer_write_index] = rawCounts;

  if (current_channel_adc0 == (NUM_CHANNELS_PER_ADC - 1)) {
    uint16_t next = (uint16_t)((buffer_write_index + 1) % BUFFER_SIZE);
    if (next == buffer_read_index) {
      pauseAcquisitionFromISR();
      in_isr0 = false;
      return;
    }
    buffer_write_index = next;
    frames_pending++;
  }

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

      float f_now[4];
      f_now[0] = ((float)rawCounts[0] - Lc1Off) * gainB11;
      f1.fval = f_now[0];
      f_now[1] = ((float)rawCounts[1] - Lc2Off) * gainB12;
      f2.fval = f_now[1];
      f_now[2] = ((float)rawCounts[2] - Lc3Off) * gainB13;
      f3.fval = f_now[2];
      f_now[3] = ((float)rawCounts[3] - Lc4Off) * gainB14;
      f4.fval = f_now[3];

    UpdateCOP0(f1.fval, f2.fval, f3.fval, f4.fval);

//    packetCounter++;
//    Timestamp = millis();
//    writeSensorStream();

   Serial.print(COPx_B1.fval); Serial.print('\t');
   Serial.print(COPy_B1.fval); Serial.print('\t');
   Serial.print(WB1); Serial.print('\t');

   Serial.print(f1.fval); Serial.print('\t');
   Serial.print(f2.fval); Serial.print('\t');
   Serial.print(f3.fval); Serial.print('\t');
   Serial.println(f4.fval); 
    processed++;
  }
  return processed;
}

// =============================== Acquisition Pause/Resume ============================
static inline void stopAcqHard() {
  detachInterrupt(digitalPinToInterrupt(DRDY0_PIN));
  ads0_stop();
  spi0EndTxn();
}

static inline void startAcqHard() {
  spi0StartTxn();
  selectDiff0(ADC0_MUX_P_PINS[0], ADC0_MUX_N_PINS[0]);
  current_channel_adc0 = 0;

  attachInterrupt(digitalPinToInterrupt(DRDY0_PIN), DRDY0_ISR, FALLING);
  ads0_start();
}

// =====================================================================================
volatile bool tare_requested = false;
void performTare() {
  stopAcqHard();

  
  Lc1Off = GetLCoff0(1,2);
  Lc2Off = GetLCoff0(3,4);
  Lc3Off = GetLCoff0(5,6);
  Lc4Off = GetLCoff0(7,8);

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
  pinMode(TEST_PIN, OUTPUT);

  pinMode(CS0_PIN, OUTPUT);
  pinMode(DRDY0_PIN, INPUT);
  pinMode(RESET0_PIN, OUTPUT);
  digitalWriteFast(CS0_PIN, HIGH);
  digitalWriteFast(RESET0_PIN, HIGH);

  digitalWriteFast(RESET0_PIN, LOW); delayMicroseconds(10); digitalWriteFast(RESET0_PIN, HIGH);
  SPI.begin();

  SPI.usingInterrupt(digitalPinToInterrupt(DRDY0_PIN));

  spi0StartTxn();
  ConfigADS0();
  // SelfOffCal_ADS0();

  Lc1Off = GetLCoff0(1,2);
  Lc2Off = GetLCoff0(3,4);
  Lc3Off = GetLCoff0(5,6);
  Lc4Off = GetLCoff0(7,8);

  InitSensorVals();
  startAcqHard();
}

void loop() {
  digitalWrite(TEST_PIN, !digitalRead(TEST_PIN));
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
