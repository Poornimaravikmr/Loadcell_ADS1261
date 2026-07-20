#ifndef variable_h
#define variable_h
#define TEST_PIN 1
#include "Arduino.h"
#include "CustomDS.h"

union FloatBytes_ud
{
  float fval;
  byte fbytes[4];
};

FloatBytes_ud f1, f2, f3, f4;
FloatBytes_ud COPx_B1, COPy_B1;  
float Lc1Off =0.0f, Lc2Off =0.0f, Lc3Off =0.0f, Lc4Off =0.0f;
float WB1 =0.0f;
unsigned long packetCounter = 0 ;
double Timestamp =0;

unsigned long prevTime = 0;
const int interval = 800;
OutDataBuffer4Float outPayload;

const uint8_t ADC0_MUX_P_PINS[] = {1, 3, 5, 7};
const uint8_t ADC0_MUX_N_PINS[] = {2, 4, 6, 8};
const uint8_t NUM_CHANNELS_PER_ADC = 4;
const uint8_t TOTAL_CHANNELS = 4;

const uint16_t BUFFER_SIZE = 256;
extern volatile int32_t adc_buffer[TOTAL_CHANNELS][BUFFER_SIZE];
extern volatile uint16_t buffer_write_index;
extern volatile uint16_t buffer_read_index;
extern volatile bool data_ready_to_process;



float gainB11 = 1.0 / 13661.32;  //16
float gainB12 = 1.0 / 13786.49;  //10
float gainB13 = 1.0 / 13938.09;  //5
float gainB14 = 1.0 / 13552.59;  //18


// float gainB11 = 1.0 / 14022.19;  //14
// float gainB12 = 1.0 / 14043.36;  //12
// float gainB13 = 1.0 / 14161.15;  //11
// float gainB14 = 1.0 / 14179.12;  //15



float L_HALF = 65.8 / 2;
float B_HALF = 30.0 / 2;
int i;

#endif
