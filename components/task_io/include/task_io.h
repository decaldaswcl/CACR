#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif




#define CHANNEL1 23
#define CHANNEL2 22
#define CHANNEL3 17
#define CHANNEL4 19
#define CHANNEL5 18
#define CHANNEL6 5
//#define CHANNEL7 17
#define WIFI_LED_PIN 2
#define AP_LED_PIN 4

//LEDc Var
#define LEDC_HS_TIMER_0          LEDC_TIMER_0
#define LEDC_HS_TIMER_1          LEDC_TIMER_1
#define LEDC_HS_TIMER_2          LEDC_TIMER_2
#define LEDC_MODE                LEDC_HIGH_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY          (1000) // Frequency in Hertz. Set frequency at 5 kHz
#define LEDC_HS_CH0_GPIO        (25)
#define LEDC_HS_CH0_CHANNEL     LEDC_CHANNEL_0
#define LEDC_HS_CH2_GPIO        (33)
#define LEDC_HS_CH2_CHANNEL     LEDC_CHANNEL_2
#define LEDC_HS_CH4_GPIO        (32)
#define LEDC_HS_CH4_CHANNEL     LEDC_CHANNEL_4

void set_alarm(int number_alarm);
bool get_ch_state(int channel);
void set_ch_state(int channel, bool state); 
void init_io();
void wifi_led_set_mode(int mode);
void ap_led_set_mode(int mode);
void led_set_color();
float get_read_temp(int termostate_number);

#ifdef __cplusplus
}
#endif
