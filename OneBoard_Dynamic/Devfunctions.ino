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

void InitSensorVals() {
  f1.fval = 0.0f; f2.fval = 0.0f; f3.fval = 0.0f; f4.fval = 0.0f;
  COPx_B1.fval = 0.0f; COPy_B1.fval = 0.0f;
  WB1 = 0.0f; 
}
