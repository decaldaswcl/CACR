#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "task_wifi.h"
#include "task_storage.h"
#include "task_system.h"
#include "task_web_server.h"

#include "task_io.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "LIVE TIM_6400_2G"
#define EXAMPLE_ESP_WIFI_PASS      "willcasa2020"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
#define EXAMPLE_ESP_WIFI_SSID_AP      "Central_Automatica"
#define EXAMPLE_ESP_WIFI_PASS_AP      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       1

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";


bool ap_connected = false;
int old = false;
wifi_ap_record_t ret_ap_info[10];
uint16_t ap_count = 0;
char ssid[32]= "NDGR";
char pass[64]= "ASDE";
int sta_ck = 0;



static void scan_done_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
        uint16_t number = 10;
        wifi_ap_record_t ap_info[10];
        
        char * ap_str[3];               
        memset(ap_info, 0, sizeof(ap_info));        
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        ESP_LOGI(TAG, "Total APs scanned = %u", ap_count); 
       for (int i = 0; (i < 10) && (i < ap_count); i++) { 
            strcpy(&ret_ap_info[i].ssid, &ap_info[i].ssid);
            ret_ap_info[i].rssi = ap_info[i].rssi;         
            strcpy(&ret_ap_info[i].bssid, &ap_info[i].bssid);
            ret_ap_info[i].primary = ap_info[i].primary;
            ret_ap_info[i].authmode = ap_info[i].authmode;  
            ESP_LOGI(TAG, "Channel \t\t%s\n", ap_info[i].ssid);
        }
            
}
static void event_handler_start_ap(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data){

    ap_led_set_mode(LED_READ_MODE);
    ESP_LOGI(TAG, "AP-start");
                                }
static void event_handler_start_sta(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    wifi_led_set_mode(LED_READ_MODE);
    ESP_LOGI(TAG, "STA-start");
      
    esp_wifi_connect();
     
}
static void event_handler_ap_disconected(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data){
    ESP_LOGI(TAG, "AP Desconectado");
    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    ap_led_set_mode(LED_READ_MODE);
    ap_connected = false;   
    esp_restart();
    esp_wifi_connect();
}
static void event_handler_sta_disconected(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{       
    
    ESP_LOGI(TAG, "STA Desconectado");
    //stop_webserver();
    if(!ap_connected){
       
        wifi_led_set_mode(LED_READ_MODE);       
    
        char ssid_key []= "SSID ";            
            ssid_key
            [4] = sta_ck + '0';
        char pass_rd []= "PASS ";            
            pass_rd[4] = sta_ck + '0';  
                     
        read_nvs_str("dadosNVS", "wifi", ssid_key, &ssid);        
        read_nvs_str("dadosNVS", "wifi", pass_rd, &pass);
        

        ESP_LOGI(TAG, "SSID: %s", ssid ); 
        if(strcmp(&ssid, "")){
        wifi_config_t wifi_config_sta = {
            .sta = { 
                .pmf_cfg = {
                    .capable = true,
                    .required = false
                },
            },
        };
        memcpy(wifi_config_sta.sta.ssid, ssid, sizeof(ssid));
        memcpy(wifi_config_sta.sta.password, pass, sizeof(pass)); 
            
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta) );
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "teste: " ); 
        }
        sta_ck++;
        if(sta_ck>4)sta_ck = 1;
        ESP_LOGI(TAG, "int: %d", sta_ck);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(esp_wifi_connect());  
    }
       
} 
static void connect_ap_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
        MAC2STR(event->mac), event->aid);
        ap_led_set_mode(LED_CONECTED_MODE);                                
        ESP_LOGI(TAG, "Ap-connected");
        start_webserver();
        esp_wifi_disconnect();
        wifi_led_set_mode(LED_OFF_MODE);
        set_ap_mode(true);
        ap_connected = true;  
}
static void event_handler_got_ip(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    wifi_led_set_mode(LED_CONECTED_MODE);
    if(sta_ck == 1){
        shift_w();
    }    
    set_alarm(1);
        int32_t auto_snpt = read_nvs_int32("dadosNVS", "clock", "autotime_sntp"); 
        char *sever;
        read_nvs_str("dadosNVS","clock", "ntp_sever", &sever);
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));       
                
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        init_sntp();
        start_webserver();

        
 }





static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    default:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}
static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}


void wifi_init(void)
{   
    //start_mdns_service();
    esp_log_level_set("wifi", ESP_LOG_WARN);
    s_wifi_event_group = xEventGroupCreate();

    
    if(esp_netif_init()){
        ESP_LOGI(TAG, "Falha na inicialização da pilha TCP/IP");
    }

    if(esp_event_loop_create_default()){
        ESP_LOGI(TAG, "Falha criação da task de eventos");
    }
    //Chama task  para criar uma estação de ligação de instância 
    // de interface de rede padrão ou AP com pilha TCP/IP
    //https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/network/esp_netif.html?highlight=esp_netif_init#wifi-default-initialization
     esp_netif_t *my_sta = esp_netif_create_default_wifi_sta();
    
    esp_netif_create_default_wifi_ap();
    
    esp_netif_dhcpc_stop(my_sta);
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 1, 22);
   	IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
   	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    esp_netif_set_ip_info(my_sta, &ip_info);

    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_STA_DISCONNECTED,
                                                        &event_handler_sta_disconected,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_STADISCONNECTED,
                                                        &event_handler_ap_disconected,
                                                        NULL,
                                                        NULL));                                                        
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_STA_START,
                                                        &event_handler_start_sta,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_START,
                                                        &event_handler_start_ap,
                                                        NULL,
                                                        NULL));                                                     
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_SCAN_DONE,
                                                        &scan_done_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler_got_ip,
                                                        NULL,
                                                        &instance_got_ip));
     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_STACONNECTED,
                                                        &connect_ap_handler,
                                                        NULL,
                                                        NULL));                                                        
    read_nvs_str("dadosNVS", "wifi", "SSID", &ssid); 
    read_nvs_str("dadosNVS", "wifi", "PASS", &pass);    
    wifi_config_t wifi_config_sta = {
        .sta = {  
            .ssid = "ss",
            .password = "12345678",           
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },        
    };

    //memcpy(wifi_config_sta.sta.ssid, ssid, sizeof(ssid));
    //memcpy(wifi_config_sta.sta.password, pass, sizeof(pass));

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID_AP,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS_AP,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ssid, pass);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ssid, pass);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    //vEventGroupDelete(s_wifi_event_group);
}



char* get_scan(){
    cJSON *root, *ap;
    char *ap_at[10];
    root = cJSON_CreateObject(); 
    cJSON_AddNumberToObject(root, "totalAP", ap_count);
    //cJSON_AddItemToObject(root, "0", ap=cJSON_CreateObject());
    //cJSON_AddStringToObject(ap, "SSID", &ret_ap_info[i].ssid);
    //cJSON_AddNumberToObject(ap, "RSSI", ret_ap_info[i].rssi);
    for (int i = 0; (i < 10) && (i < ap_count); i++) { 
    
    itoa(i,&ap_at,10);
    cJSON_AddItemToObject(root, ap_at, ap=cJSON_CreateObject());    
    cJSON_AddStringToObject(ap, "SSID", &ret_ap_info[i].ssid);
    cJSON_AddNumberToObject(ap, "RSSI", ret_ap_info[i].rssi);
    cJSON_AddNumberToObject(ap, "Authmode", ret_ap_info[i].authmode);
   
    strcmp((char*)ret_ap_info[i].ssid, &ssid)== 0?cJSON_AddBoolToObject(ap, "conected", true) : cJSON_AddBoolToObject(ap, "conected", false);
    
    for (size_t y = 0; y < 4; y++)
    {
        char ssid_rd[32];
        char ssid_key[]= "SSID ";
        ssid_key[4] = y + '0';   
               
        read_nvs_str("dadosNVS", "wifi", ssid_key, &ssid_rd); 

        ESP_LOGI(TAG, "SSID Atual: %s SSID lido: %s ", ssid,  ssid_rd);

        if(strcmp( (char*)ret_ap_info[i].ssid, &ssid_rd) == 0){
            cJSON_AddBoolToObject(ap, "save", true); 
            y=3;         
        }else{
            cJSON_AddBoolToObject(ap, "save", false);            
        }

    }
    
    
    //ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
    }
    char *retstr = cJSON_Print(root);
    //strcpy(ret_scan, retstr);
    
    
    cJSON_Delete(root);
    return retstr;
}

void disconect_wifi(void){
    
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(1000));
}


void start_scan_ap(void){
    esp_wifi_scan_start(NULL, true);
}

void wifi_new_connect(char *w_ssid, char *w_password ){
    //disconect_wifi();
    wifi_config_t wifi_config_sta = {
            .sta = {             
                .pmf_cfg = {
                    .capable = true,
                    .required = false
                },
            },        
        };
        memcpy(wifi_config_sta.sta.ssid, "Will", sizeof("Will"));
        memcpy(wifi_config_sta.sta.password, "12345678", sizeof("12345678")); 

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta) );
        //ESP_ERROR_CHECK(esp_wifi_start());

        ESP_ERROR_CHECK(esp_wifi_connect());

}

