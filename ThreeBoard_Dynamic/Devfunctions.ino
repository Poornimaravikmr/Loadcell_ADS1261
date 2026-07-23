float GetLCoff0(uint8_t ainp, uint8_t ainn) {
  detachInterrupt(digitalPinToInterrupt(DRDY0_PIN));

  spi0StartTxn();
  selectDiff0(ainp, ainn);

  noInterrupts();
  offcap_p0 = ainp; offcap_n0 = ainn;
  offcap_acc0 = 0;
  offcap_got0 = 0;
  offcap_need0 = 2400;
  offcap_done0 = false;
  offcap0 = true;
  interrupts();

  attachInterrupt(digitalPinToInterrupt(DRDY0_PIN), DRDY0_ISR, FALLING);
  ads0_start();

  const unsigned long t0 = millis();
  while (!offcap_done0) {
    if (millis() - t0 > 2000UL) break;
    yield();
  }

  ads0_stop();
  detachInterrupt(digitalPinToInterrupt(DRDY0_PIN));

  noInterrupts();
  bool done = offcap_done0;
  int64_t acc = offcap_acc0;
  uint16_t got = offcap_got0;
  offcap0 = false;
  interrupts();

  spi0EndTxn();

  if (!done || got == 0) return 0.0f;
  return (float)((double)acc / (double)got);
}

float GetLCoff1(uint8_t ainp, uint8_t ainn) {
  detachInterrupt(digitalPinToInterrupt(DRDY1_PIN));

  spi1StartTxn();
  selectDiff1(ainp, ainn);

  noInterrupts();
  offcap_p1 = ainp; offcap_n1 = ainn;
  offcap_acc1 = 0;
  offcap_got1 = 0;
  offcap_need1 = 2400;
  offcap_done1 = false;
  offcap1 = true;
  interrupts();

  attachInterrupt(digitalPinToInterrupt(DRDY1_PIN), DRDY1_ISR, FALLING);
  ads1_start();

  const unsigned long t0 = millis();
  while (!offcap_done1) {
    if (millis() - t0 > 2000UL) break;
    yield();
  }

  ads1_stop();
  detachInterrupt(digitalPinToInterrupt(DRDY1_PIN));

  noInterrupts();
  bool done = offcap_done1;
  int64_t acc = offcap_acc1;
  uint16_t got = offcap_got1;
  offcap1 = false;
  interrupts();

  spi1EndTxn();

  if (!done || got == 0) return 0.0f;
  return (float)((double)acc / (double)got);
}

float GetLCoff2(uint8_t ainp, uint8_t ainn) {
  detachInterrupt(digitalPinToInterrupt(DRDY2_PIN));

  spi2StartTxn();
  selectDiff2(ainp, ainn);

  noInterrupts();
  offcap_p2 = ainp; offcap_n2 = ainn;
  offcap_acc2 = 0;
  offcap_got2 = 0;
  offcap_need2 = 2400;
  offcap_done2 = false;
  offcap2 = true;
  interrupts();

  attachInterrupt(digitalPinToInterrupt(DRDY2_PIN), DRDY2_ISR, FALLING);
  ads2_start();

  const unsigned long t0 = millis();
  while (!offcap_done2) {
    if (millis() - t0 > 2000UL) break;
    yield();
  }

  ads2_stop();
  detachInterrupt(digitalPinToInterrupt(DRDY2_PIN));

  noInterrupts();
  bool done = offcap_done2;
  int64_t acc = offcap_acc2;
  uint16_t got = offcap_got2;
  offcap2 = false;
  interrupts();

  spi2EndTxn();

  if (!done || got == 0) return 0.0f;
  return (float)((double)acc / (double)got);
}

//----------------------------------------------------------------------------------------

void UpdateCOP0(float F1, float F2, float F3, float F4) {
 
  WB1 = F1 + F2 + F3 + F4;

  if (WB1 > 1.0f) {
    COPy_B1.fval = (((F1 + F2) - (F3 + F4)) / WB1) * L_HALF;
    COPx_B1.fval = (((F2 + F3) - (F1 + F4)) / WB1) * B_HALF;
    
  } else {
    COPx_B1.fval = 0.0f;
    COPy_B1.fval = 0.0f;
  }
}

void UpdateCOP1(float F5, float F6, float F7, float F8) {
 
  WB2 = F5 + F6 + F7 + F8;

  if (WB2 > 1.0f) {
    COPy_B2.fval = (((F5 + F6) - (F7 + F8)) / WB2) * L_HALF;
    COPx_B2.fval = (((F6 + F7) - (F5 + F8)) / WB2) * B_HALF;
    
  } else {
    COPx_B2.fval = 0.0f;
    COPy_B2.fval = 0.0f;
  }
}

void UpdateCOP2(float F9, float F10, float F11, float F12) {
 
  WB3 = F9 + F10 + F11 + F12;

  if (WB3 > 1.0f) {
    COPy_B3.fval = (((F9 + F10) - (F11 + F12)) / WB3) * L_HALF;
    COPx_B3.fval = (((F10 + F11) - (F9 + F12)) / WB3) * B_HALF;
    
  } else {
    COPx_B3.fval = 0.0f;
    COPy_B3.fval = 0.0f;
  }
}

void InitSensorVals() {
  f1.fval = 0.0f; f2.fval = 0.0f; f3.fval = 0.0f; f4.fval = 0.0f;
  f5.fval = 0.0f; f6.fval = 0.0f; f7.fval = 0.0f; f8.fval = 0.0f;
  f9.fval = 0.0f; f10.fval = 0.0f; f11.fval = 0.0f; f12.fval = 0.0f;
  COPx_B1.fval = 0.0f; COPy_B1.fval = 0.0f;
  COPx_B2.fval = 0.0f; COPy_B2.fval = 0.0f;
  COPx_B3.fval = 0.0f; COPy_B3.fval = 0.0f;
  WB1 = 0.0f; 
  WB2 = 0.0f; 
  WB3 = 0.0f; 
}
