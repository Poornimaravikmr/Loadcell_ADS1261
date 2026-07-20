#ifndef variable_h
#define variable_h

#include "Arduino.h"
#include "CustomDS.h"

union FloatBytes_ud
{
  float fval;
  byte fbytes[4];
};

FloatBytes_ud f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12;
FloatBytes_ud COPx_B1, COPy_B1, COPx_B2, COPy_B2, COPx_B3, COPy_B3; 
float Lc1Off =0.0f, Lc2Off =0.0f, Lc3Off =0.0f, Lc4Off =0.0f, Lc5Off =0.0f, Lc6Off =0.0f, Lc7Off =0.0f, Lc8Off =0.0f, Lc9Off =0.0f, Lc10Off =0.0f, Lc11Off =0.0f, Lc12Off =0.0f;
float WB1 =0.0f, WB2 =0.0f, WB3 =0.0f;
unsigned long packetCounter = 0 ;
double Timestamp =0;

unsigned long prevTime = 0;
const int interval = 800;
OutDataBuffer4Float outPayload;

const uint8_t ADC0_MUX_P_PINS[] = {3, 5, 7, 9 };
const uint8_t ADC0_MUX_N_PINS[] = {4, 6, 8, 10};
const uint8_t ADC1_MUX_P_PINS[] = {1, 3, 5, 7};
const uint8_t ADC1_MUX_N_PINS[] = {2, 4, 6, 8};
const uint8_t NUM_CHANNELS_PER_ADC = 4;
const uint8_t TOTAL_CHANNELS = 8;

const uint16_t BUFFER_SIZE = 512;
extern volatile int32_t adc_buffer[TOTAL_CHANNELS][BUFFER_SIZE];
extern volatile uint16_t buffer_write_index;
extern volatile uint16_t buffer_read_index;
extern volatile bool data_ready_to_process;

float gainB11 = 1.0 / 13382.7;
float gainB12 = 1.0 / 13367.6;
float gainB13 = 1.0 / 13892.7;
float gainB14 = 1.0 / 13944.4;
float gainB21 = 1.0 / 13492.0;
float gainB22 = 1.0 / 13956.0;   
float gainB23 = 1.0 / 14334.0;
float gainB24 = 1.0 / 13960.0;


float L_HALF = 65.8 / 2;
float B_HALF = 30.0 / 2;
int i;

#endif
