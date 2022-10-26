
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "task_io.h"
#include <time.h>
#include <sys/time.h>
#include "task_storage.h"

xSemaphoreHandle mutex;

bool out_ch1_state = false;
bool out_ch2_state = false;
bool out_ch3_state = false;
bool out_ch4_state = false;
bool out_ch5_state = false;
bool out_ch6_state = false;
bool out_ch7_state = false;
bool wifi_out = false;
bool ap_out = false;
int  Wifi_led_mode = 0;
int  ap_led_mode = 0;
 

u_int8_t timers_state = 0x0f;
int week = 0x00;


void set_alarm(int number_alarm){
    /*
    if(number_alarm == 0){
        gpio_set_level(CHANNEL2,0);    
    }else if(number_alarm == 1){
        gpio_set_level(CHANNEL2,1); 
    }else if(number_alarm == 2){
     gpio_set_level(CHANNEL2,1);   
    }*/
}



bool get_ch_state(int channel){
     bool val = 0;    
    if(xSemaphoreTake(mutex, 200 / portTICK_PERIOD_MS)){
        if(channel==1)val=out_ch1_state;
        if(channel==2)val=out_ch2_state;
        if(channel==3)val=out_ch3_state;
        if(channel==4)val=out_ch4_state;
        if(channel==5)val=out_ch5_state;
        if(channel==6)val=out_ch6_state;
        if(channel==7)val=out_ch7_state;
    }
    xSemaphoreGive(mutex);  
    return val;
}
void set_ch_state(int channel, bool state){
      
    if(xSemaphoreTake(mutex, 200 / portTICK_PERIOD_MS)){
        if(channel==1)out_ch1_state = state;
        if(channel==2)out_ch2_state = state;
        if(channel==3)out_ch3_state = state;
        if(channel==4)out_ch4_state = state;
        if(channel==5)out_ch5_state = state;
        if(channel==6)out_ch6_state = state;
        if(channel==7)out_ch7_state = state;
    }
    xSemaphoreGive(mutex);  
    
}

void wifi_led_set_mode(int mode){

    Wifi_led_mode = mode;
}
void ap_led_set_mode(int mode){

   ap_led_mode = mode;
}

static void output_set_task(void *arg){
    while (true)
    {
        if(true){

        }
        gpio_set_level(CHANNEL1, get_ch_state(1)); 
        gpio_set_level(CHANNEL2, get_ch_state(2));
        gpio_set_level(CHANNEL3, get_ch_state(3));
        gpio_set_level(CHANNEL4, get_ch_state(4));
        gpio_set_level(CHANNEL5, get_ch_state(5));
        gpio_set_level(CHANNEL6, get_ch_state(6));
        gpio_set_level(CHANNEL7, get_ch_state(7));
        
        if(Wifi_led_mode==0) wifi_out = false;
        if(Wifi_led_mode==1) wifi_out = !wifi_out;
        if(Wifi_led_mode==2) wifi_out = true;
        if(ap_led_mode==0) ap_out = false;
        if(ap_led_mode==1) ap_out = !ap_out;
        if(ap_led_mode==2) ap_out = true;

        gpio_set_level(WIFI_LED_PIN, wifi_out);
        gpio_set_level(AP_LED_PIN, ap_out);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}

void timer_set(){

}

static void timer_task(void *arg){
    char strftime_buf[64];
    time_t now;
    struct tm timeinfo;
    while (true)
    {    


        time(&now);
        setenv("TZ", "UTC+3", 1);
        tzset();
        //ESP_LOGI(TAG, "Valor estado %d", timers_state);
        localtime_r(&now, &timeinfo);

        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        //ESP_LOGI(TAG, "The current date/time in Sao paulo is: %s", strftime_buf);
        
        int no_ch = 0b1111111;
        for (size_t i = 1; i < 5; i++)
        {
            char str_tm []= "timer ";            
            str_tm[5] = i + '0';
            int val = read_nvs_int32("dadosNVS", str_tm, "channel");
            int week = read_nvs_int32("dadosNVS", str_tm, "week");
            int i_hour_st = read_nvs_int32("dadosNVS", str_tm, "hour_st"); 
            int i_min_st = read_nvs_int32("dadosNVS", str_tm, "min_st"); 
            int i_hour_en = read_nvs_int32("dadosNVS", str_tm, "hour_en"); 
            int i_min_en = read_nvs_int32("dadosNVS", str_tm, "min_en");  
            if(val != 0){
                
                for (size_t y = 0; y < 7; y++)
                {                
                    if(week & (1 << y) && y == timeinfo.tm_wday){
                        if((timeinfo.tm_hour == i_hour_en && timeinfo.tm_min < i_min_en) || (timeinfo.tm_hour == i_hour_st && timeinfo.tm_min >= i_min_st) || (timeinfo.tm_hour > i_hour_st && timeinfo.tm_hour < i_hour_en)){
                            
                            if(val & (1 << 0)){set_ch_state(1, true);
                                no_ch &= ~(1 << 0);
                            }
                            if(val & (1 << 1)){
                                set_ch_state(2, true);
                                no_ch &= ~(1 << 1);
                            }
                            if(val & (1 << 2)){
                                set_ch_state(3, true);
                                no_ch &= ~(1 << 2);
                            }
                            if(val & (1 << 3)){
                                set_ch_state(4, true);
                                no_ch &= ~(1 << 3);
                            }
                            if(val & (1 << 4)){
                                set_ch_state(5, true);
                                no_ch &= ~(1 << 4);
                            }
                            if(val & (1 << 5)){
                                set_ch_state(6, true);
                                no_ch &= ~(1 << 5);
                            }
                            if(val & (1 << 6)){
                                set_ch_state(7, true);
                                no_ch &= ~(1 << 6);
                            }
                        }                        
                    }
                }
                        
            }
            



        }

        if(no_ch & (1 << 0))set_ch_state(1, false);
        if(no_ch & (1 << 1))set_ch_state(2, false);
        if(no_ch & (1 << 2))set_ch_state(3, false);
        if(no_ch & (1 << 3))set_ch_state(4, false);
        if(no_ch & (1 << 4))set_ch_state(5, false);
        if(no_ch & (1 << 5))set_ch_state(6, false);
        if(no_ch & (1 <<6))set_ch_state(7, false);

        int val = (rand() % 8000);

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, val ));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
void init_io(){
    
    mutex = xSemaphoreCreateMutex();
    //char strftime_buf[64];
    //time_t now;
    //struct tm timeinfo;
    //time(&now);
    //setenv("TZ", "UTC+3", 1);
    //tzset();
    //localtime_r(&now, &timeinfo);
    //strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    //ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);
    
    
    (true)?timers_state |= 0x01 : (timers_state &= 0xfe);
    (true)?timers_state |= 0x02 : (timers_state &= 0xfd);
    (true)?timers_state |= 0x04 : (timers_state &= 0xfb);
    (true)?timers_state |= 0x08 : (timers_state &= 0xf7);
    (true)?timers_state |= 0x10 : (timers_state &= 0xef);
    (true)?timers_state |= 0x20 : (timers_state &= 0xdf);
    (false)?timers_state |= 0x40 : (timers_state &= 0x7f);
    (false)?timers_state |= 0x80 : (timers_state &= 0xff);
    /*
    val[0] = read_nvs_int32("dadosNVS", "timer1", "channel");
    int val [7] = { };
    uint8_t ch[] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40};
    for (size_t i = 0; i < 8; i++)
    {
            switch (val[i])
        {
        case 0:
            timers_state |= ch[i];
            break;
        case 1:
            timers_state |= ch[i];
            break;
        case 2:
            timers_state |= ch[i];
            break;
        case 3:
            timers_state |= ch[i];
            break;
        case 4:
            timers_state |= ch[i];
            break;
        case 5:
            timers_state |= ch[i];
            break;
        default:
            
            break;
         } 
    }
    */
    
    gpio_reset_pin(CHANNEL1);
    gpio_reset_pin(CHANNEL2);
    gpio_reset_pin(CHANNEL3);
    gpio_reset_pin(CHANNEL4);
    gpio_reset_pin(CHANNEL5);
    gpio_reset_pin(CHANNEL6);
    gpio_reset_pin(CHANNEL7);
    gpio_reset_pin(WIFI_LED_PIN);
    gpio_reset_pin(AP_LED_PIN);
    gpio_set_direction(CHANNEL1, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL2, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL3, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL4, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL5, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL6, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL7, GPIO_MODE_OUTPUT);
    gpio_set_direction(WIFI_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(AP_LED_PIN, GPIO_MODE_OUTPUT);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    

    ledc_timer_config_t ledc_timer_0 = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_HS_TIMER_0,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };    
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_0));
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer_1 = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_HS_TIMER_1,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };    
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_1));
    ledc_timer_config_t ledc_timer_2 = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_HS_TIMER_2,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_2));

    ledc_channel_config_t ledc_channel_0 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_HS_CH0_CHANNEL,
        .timer_sel      = LEDC_HS_TIMER_0,        
        .gpio_num       = LEDC_HS_CH0_GPIO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_0));
   ledc_channel_config_t ledc_channel_1 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_HS_CH2_CHANNEL,
        .timer_sel      = LEDC_HS_TIMER_1,        
        .gpio_num       = LEDC_HS_CH2_GPIO,
        .duty           = 1000, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_1));
    ledc_channel_config_t ledc_channel_2 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_HS_CH4_CHANNEL,
        .timer_sel      = LEDC_HS_TIMER_2,        
        .gpio_num       = LEDC_HS_CH4_GPIO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_2));
    ledc_fade_func_install(0);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    ledc_set_fade_with_time(ledc_channel_2.speed_mode,ledc_channel_2.channel, 5000 ,LEDC_FADE_NO_WAIT);
    ledc_fade_start(ledc_channel_2.speed_mode,ledc_channel_2.channel,LEDC_FADE_NO_WAIT);

    
    xTaskCreate(output_set_task,"output_set_task", 2024 ,"task1", 10,NULL);
    xTaskCreate(timer_task,"timer_task", 2024 ,"task2", 10,NULL);
}

