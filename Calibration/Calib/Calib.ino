#include <SPI.h>

// ===================================================================
// LOAD CELL FIRMWARE WITH SERIAL COMMAND INTERFACE
// Upload this ONCE. All channel selection, taring, and calibration
// is then done live from the Python companion script over Serial.
// ===================================================================

#define CS_PIN     10
#define DRDY_PIN   2
#define RESET_PIN  3

SPISettings spiSettings(5000000, MSBFIRST, SPI_MODE1);

// ---------------- Calibration----
float calM = 1.0;     // slope:  weight = (adc - calC) / calM
float calC = 0.0;     // intercept / tare offset

// ---------------- Channel map ---------------------------------------
// INPMUX = (AINP << 4) | AINN.  Fill in / verify CH9 once confirmed.
// Index:      0     1     2     3     4
// Label:    CH12  CH34  CH56  CH78  CH9
const uint8_t channelTable[5] = {0x12, 0x34, 0x56, 0x78, 0x90};
const char*   channelNames[5] = {"CH12", "CH34", "CH56", "CH78", "CH9"};
int currentChannelIndex = 0; // default CH34, matches original code

// ---------------- Streaming mode -------------------------------------
enum StreamMode { STREAM_RAW, STREAM_WEIGHT, STREAM_OFF };
StreamMode streamMode = STREAM_RAW;

// ---------------- Low level SPI helpers -------------------------------
void spiBegin() { digitalWrite(CS_PIN, LOW); }
void spiEnd()   { digitalWrite(CS_PIN, HIGH); }

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
  SPI.transfer(0x00); // echo byte, discarded
  uint8_t b2 = SPI.transfer(0x00);
  uint8_t b1 = SPI.transfer(0x00);
  uint8_t b0 = SPI.transfer(0x00);
  spiEnd();

  int32_t val = ((int32_t)b2 << 16) | ((int32_t)b1 << 8) | (int32_t)b0;
  if (val & 0x800000) {
    val |= 0xFF000000;
  }
  return val;
}

bool getFreshSample(int32_t &adc_counts) {
  if (!waitForDRDY(1)) {
    return false;
  }
  adc_counts = readADCDataOnce();
  return true;
}

// ---------------- Channel select --------------------------------------
void setChannel(int index) {
  if (index < 0 || index > 4) return;
  currentChannelIndex = index;
  writeRegister(0x11, channelTable[index]); // INPMUX
  delay(5); // let the input settle after mux switch
  Serial.print("OK:CHANNEL:");
  Serial.println(channelNames[index]);
}

// ---------------- Tare --------------------------------------------------
// Averages N fresh samples and stores result as the new offset (calC).
void doTare(int numSamples) {
  // flush a few stale samples first
  int32_t tmp;
  for (int i = 0; i < 10; i++) {
    getFreshSample(tmp);
  }

  long long acc = 0;
  int good = 0;
  for (int i = 0; i < numSamples; i++) {
    if (getFreshSample(tmp)) {
      acc += tmp;
      good++;
    } else {
      break;
    }
  }

  if (good == 0) {
    Serial.println("ERR:TARE_NO_SAMPLES");
    return;
  }

  calC = (float)acc / (float)good;
  Serial.print("OK:TARE:");
  Serial.println(calC, 6);
}

// ---------------- Calibration set ---------------------------------------
void setCalibration(float m, float c) {
  calM = m;
  calC = c;
  Serial.print("OK:CAL:");
  Serial.print(calM, 6);
  Serial.print(",");
  Serial.println(calC, 6);
}

// ---------------- Command parser -----------------------------------------
// Recognized commands (newline terminated):
//   CH:0 .. CH:4          select channel by index (see channelTable)
//   TARE                  tare using default 2400 samples
//   TARE:<n>              tare using n samples
//   CAL:<M>,<C>            set slope/intercept
//   STREAM:RAW            stream raw ADC counts (default)
//   STREAM:WEIGHT         stream computed weight ((adc-C)/M)
//   STREAM:OFF            stop streaming
//   STATUS                print current channel/cal/stream state
void handleCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.startsWith("CH:")) {
    int idx = cmd.substring(3).toInt();
    setChannel(idx);

  } else if (cmd.startsWith("TARE")) {
    int n = 2400;
    int colon = cmd.indexOf(':');
    if (colon != -1) {
      n = cmd.substring(colon + 1).toInt();
      if (n <= 0) n = 2400;
    }
    doTare(n);

  } else if (cmd.startsWith("CAL:")) {
    String rest = cmd.substring(4);
    int comma = rest.indexOf(',');
    if (comma == -1) {
      Serial.println("ERR:CAL_FORMAT");
      return;
    }
    float m = rest.substring(0, comma).toFloat();
    float c = rest.substring(comma + 1).toFloat();
    if (m == 0.0) {
      Serial.println("ERR:CAL_ZERO_SLOPE");
      return;
    }
    setCalibration(m, c);

  } else if (cmd == "STREAM:RAW") {
    streamMode = STREAM_RAW;
    Serial.println("OK:STREAM:RAW");

  } else if (cmd == "STREAM:WEIGHT") {
    streamMode = STREAM_WEIGHT;
    Serial.println("OK:STREAM:WEIGHT");

  } else if (cmd == "STREAM:OFF") {
    streamMode = STREAM_OFF;
    Serial.println("OK:STREAM:OFF");

  } else if (cmd == "STATUS") {
    Serial.print("OK:STATUS:");
    Serial.print(channelNames[currentChannelIndex]);
    Serial.print(",M=");
    Serial.print(calM, 6);
    Serial.print(",C=");
    Serial.print(calC, 6);
    Serial.print(",STREAM=");
    Serial.println(streamMode == STREAM_RAW ? "RAW" : streamMode == STREAM_WEIGHT ? "WEIGHT" : "OFF");

  } else {
    Serial.print("ERR:UNKNOWN_CMD:");
    Serial.println(cmd);
  }
}

// ---------------- Setup / loop --------------------------------------------
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
  writeRegister(0x11, channelTable[currentChannelIndex]); // default channel

  spiBegin();
  SPI.transfer(0x08); // start conversions
  spiEnd();

  Serial.println("OK:READY");
}

void loop() {
  // --- handle any incoming command, non-blocking line read ---
  static String inputBuffer;
  while (Serial.available() > 0) {
    char ch = Serial.read();
    if (ch == '\n') {
      handleCommand(inputBuffer);
      inputBuffer = "";
    } else if (ch != '\r') {
      inputBuffer += ch;
    }
  }

  // --- stream data if enabled ---
  if (streamMode == STREAM_OFF) {
    return;
  }

  int32_t adc_now = 0;
  if (!getFreshSample(adc_now)) {
    return; // no fresh sample yet, try again next loop without spamming errors
  }

  if (streamMode == STREAM_RAW) {
    Serial.println(adc_now);
  } else if (streamMode == STREAM_WEIGHT) {
    float weight = ((float)adc_now - calC) / calM;
    Serial.println(weight, 6);
  }
}
