
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "task_io.h"
#include <time.h>
#include <sys/time.h>
#include "task_storage.h"

static const char *TAG = "I/O";

xSemaphoreHandle mutex;
int led_red = 0;
int led_green = 0;
int led_blue = 0;

#define DEFAULT_VREF    1072        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   128 
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel_0 = ADC1_CHANNEL_0;
static const adc_channel_t channel_1 = ADC1_CHANNEL_3;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

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
int ap_led_mode = 0;

float temp_1, temp_2 = 0.0;
ledc_channel_config_t ledc_channel_0 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_HS_CH0_CHANNEL,
        .timer_sel      = LEDC_HS_TIMER_0,        
        .gpio_num       = LEDC_HS_CH0_GPIO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
ledc_channel_config_t ledc_channel_1 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_HS_CH2_CHANNEL,
        .timer_sel      = LEDC_HS_TIMER_1,        
        .gpio_num       = LEDC_HS_CH2_GPIO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
ledc_channel_config_t ledc_channel_2 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_HS_CH4_CHANNEL,
        .timer_sel      = LEDC_HS_TIMER_2,        
        .gpio_num       = LEDC_HS_CH4_GPIO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };

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
        //gpio_set_level(CHANNEL7, get_ch_state(7));
        
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

void led_set_brightness(){
     float perc = read_nvs_int32("dadosNVS", "led", "brightness");
     perc /= 100;
        led_red *= perc;
        led_green *= perc;
        led_blue *= perc;
}

void led_set_color(){
    
    int hex_color = read_nvs_int32("dadosNVS", "led", "rgb");
    int bb = 0xff;
    int gb = 0xff00;
    int rb = 0xff0000;
    int b = hex_color;
    int g = hex_color;
    int r = hex_color;
    b &= bb;
    led_blue = 0;
    led_blue = (b*8191)/255;
    g &= gb;
    g /= 0xff;
    led_green = (g*8191)/256;
    r &= rb;
    r /= 0xffff;
    led_red = (r*8191)/255;
    
    led_set_brightness();
    ESP_LOGI(TAG, "R: %d, G: %d, B: %d", r, g, b);
}
static void led_task(void *arg){
    led_set_color();
    while (true){    
        
        ledc_set_fade_with_time(ledc_channel_0.speed_mode,ledc_channel_0.channel, led_red,1000);
        ledc_fade_start(ledc_channel_0.speed_mode,ledc_channel_0.channel, LEDC_FADE_NO_WAIT);
        ledc_set_fade_with_time(ledc_channel_1.speed_mode,ledc_channel_1.channel, led_green,1000);
        ledc_fade_start(ledc_channel_1.speed_mode,ledc_channel_1.channel, LEDC_FADE_NO_WAIT);
        ledc_set_fade_with_time(ledc_channel_2.speed_mode,ledc_channel_2.channel, led_blue,1000);
        ledc_fade_start(ledc_channel_2.speed_mode,ledc_channel_2.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }
}

float get_read_temp(int termostate_number){

    if(termostate_number==1) return temp_1;
    if(termostate_number==2) return temp_2;
    return 0;
}

float get_temp_term(int ntc_val, int beta, int adc_in , int adc_total){
    if(adc_in ==0 ) return 0;
    //if(adc_total==adc_in) return 0;
    if(adc_in > 3100) return 0;
    float ret = ((ntc_val*adc_in)/(adc_total-adc_in));  
    
    ret /= ntc_val;  
    ret = log(ret);
    ret = ret / beta;
    ret =  ret +(1/298.15);
    ret = 1/ret;
    ret = (ret - 273.15);                         
    return ret;
}

static void termostate_task(void *arg){
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
    bool cross1 = false;
    bool cross2 = false;

    while (1){
        
        int beta_1 = read_nvs_int32("dadosNVS", "termostate1", "beta");
        int beta_2 = read_nvs_int32("dadosNVS", "termostate2", "beta");
        int temp_set1 = read_nvs_int32("dadosNVS",  "termostate1", "temp");
        int temp_set2 = read_nvs_int32("dadosNVS",  "termostate2", "temp");
        int offset = 2;
        
                
        uint32_t adc_reading = 0;
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++) {            
                adc_reading += adc1_get_raw((adc1_channel_t)channel_0);
        }    
        adc_reading /= NO_OF_SAMPLES;                
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);                      
        temp_1 = get_temp_term(10000, beta_1, voltage, 3300);

        adc_reading = 0;
        voltage = 0;
        for (int i = 0; i < NO_OF_SAMPLES; i++) {            
                adc_reading += adc1_get_raw((adc1_channel_t)channel_1);
        }   
        adc_reading /= NO_OF_SAMPLES;    
        
        voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        temp_2 = get_temp_term(10000, beta_2, voltage, 3300);
        //ESP_LOGI(TAG, "Voltage: %d", voltage ); 
       // ESP_LOGI(TAG, "temperatura lida: %f set: %d", temp_1, temp_set1);
        
             

        for (size_t i = 1; i < 3; i++)
        {
            char str_term []= "termostate ";            
            str_term[10] = i + '0';               

            int channel = read_nvs_int32("dadosNVS",  str_term, "channel");
            int mode = read_nvs_int32("dadosNVS", str_term,  "mode");
            float set = read_nvs_int32("dadosNVS", str_term,  "temp");
            float offset = read_nvs_int32("dadosNVS", str_term, "offset");
            if(offset == 0) offset = 0.5;
            float max = set+offset;
            float min = set-offset;
            
            
            for (size_t y = 0; y < 7; y++){
                int val = y+1;
                if(channel != 0){
                    //ESP_LOGI(TAG, "termostato: %s set: %f max: %f min %f", str_term, set ,max ,min);
                    if(i==1){
                        if(channel & (1 << y)){
                            if(!mode){
                                if( temp_1 > max) cross1 = true;
                                if( temp_1 < min) cross1 = false;        
                                (!cross1)?set_ch_state(val, true):set_ch_state(val, false); 
                            }else{
                                if( temp_1 < min) cross1 = true;
                                if( temp_1 > max) cross1 = false;        
                                (!cross1)?set_ch_state(val, true):set_ch_state(val, false);
                            }
                            
                        }
                    }else{
                       if(channel & (1 << y)){
                            if(!mode){
                                if( temp_2 > max) cross2 = true;
                                if( temp_2 < min) cross2 = false;        
                                (!cross2)?set_ch_state(val, true):set_ch_state(val, false); 
                            }else{
                                if( temp_2 < min) cross2 = true;
                                if( temp_2 > max) cross2 = false;        
                                (!cross2)?set_ch_state(val, true):set_ch_state(val, false);
                            }
                           
                        }
                            
                    }
                }
                    
            }
        }
            

            
            

        
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
static void timer_task(void *arg){
    char strftime_buf[64];
    time_t now;
    struct tm timeinfo;
    struct tm timeinfo_start;
    struct tm timeinfo_end;
    
    while (true)
    {    
        time(&now);
        setenv("TZ", "UTC+3", 1);
        tzset();
        //ESP_LOGI(TAG, "Valor estado %d", timers_state);
        localtime_r(&now, &timeinfo);
        localtime_r(&now, &timeinfo_start);
        localtime_r(&now, &timeinfo_end);

        int ch_tr_1 = read_nvs_int32("dadosNVS", "termostate1", "channel");
        int ch_tr_2 = read_nvs_int32("dadosNVS", "termostate2", "channel");
        
        int no_ch = 0b1111111;
         no_ch ^= ch_tr_1;
         no_ch ^= ch_tr_2;

        for (size_t i = 1; i < 5; i++)
        {
            char str_tm []= "timer ";            
            str_tm[5] = i + '0';
            int val = read_nvs_int32("dadosNVS", str_tm, "channel");
            int week = read_nvs_int32("dadosNVS", str_tm, "week");
            timeinfo_start.tm_hour = read_nvs_int32("dadosNVS", str_tm, "hour_st"); 
            timeinfo_start.tm_min = read_nvs_int32("dadosNVS", str_tm, "min_st"); 
            timeinfo_end.tm_hour = read_nvs_int32("dadosNVS", str_tm, "hour_en"); 
            timeinfo_end.tm_min = read_nvs_int32("dadosNVS", str_tm, "min_en");  
            timeinfo_start.tm_sec = 0;
            timeinfo_end.tm_sec = 0;    
            
            if(timeinfo.tm_year > 100){
                              
                double seconds_start = difftime(mktime(&timeinfo), mktime(&timeinfo_start) );
                double seconds_end = difftime(mktime(&timeinfo), mktime(&timeinfo_end) );                
                //ESP_LOGI(TAG,"timer ativo: %d", timeinfo.tm_year);
                //ESP_LOGI(TAG, "%.f seconds end  .\n", seconds_end);
                if( val != 0){
                    for (size_t y = 0; y < 7; y++)
                    {            
                        if(week & (1 << y) && y == timeinfo.tm_wday){                         
                            if(seconds_start > 0  && seconds_end < 0){
                                //ESP_LOGI(TAG," hora atual: %d:%d canal: %d ", timeinfo.tm_hour, timeinfo.tm_min, i );
                              
                                if(val & (1 << 0)){
                                    set_ch_state(1, true);
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
        }

        if(no_ch & (1 << 0))set_ch_state(1, false);
        if(no_ch & (1 << 1))set_ch_state(2, false);
        if(no_ch & (1 << 2))set_ch_state(3, false);
        if(no_ch & (1 << 3))set_ch_state(4, false);
        if(no_ch & (1 << 4))set_ch_state(5, false);
        if(no_ch & (1 << 5))set_ch_state(6, false);
        if(no_ch & (1 << 6))set_ch_state(7, false);        
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
    //gpio_reset_pin(CHANNEL7);
    gpio_reset_pin(WIFI_LED_PIN);
    gpio_reset_pin(AP_LED_PIN);
    gpio_set_direction(CHANNEL1, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL2, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL3, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL4, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL5, GPIO_MODE_OUTPUT);
    gpio_set_direction(CHANNEL6, GPIO_MODE_OUTPUT);
    //gpio_set_direction(CHANNEL7, GPIO_MODE_OUTPUT);
    gpio_set_direction(WIFI_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(AP_LED_PIN, GPIO_MODE_OUTPUT);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    adc1_config_width(width);
    
    adc1_config_channel_atten(channel_0, atten);
    adc1_config_channel_atten(channel_1, atten);

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

    
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_0));
    
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_1));
    
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_2));
    ledc_fade_func_install(0);
   
    
    xTaskCreate(output_set_task,"output_set_task", 2024 ,"task1", 10,NULL);
    xTaskCreate(timer_task,"timer_task", 3024 ,"task2", 10,NULL);
    xTaskCreate(led_task,"led_task", 2024 ,"task3", 10,NULL);
    xTaskCreate(termostate_task,"termostate_task", 2024 ,"task4", 10,NULL);
    ESP_LOGI(TAG,"Task IO init");
}

