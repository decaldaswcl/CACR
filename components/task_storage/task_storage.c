#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_http_server.h"

#include "task_storage.h"
#include "task_web_server.h"

static const char *TAGs="SPIffs";
static const char *TAGn="NVS";

xSemaphoreHandle mutex;

#define STORAGE_NAMESPACE "dadosNVS"

esp_err_t storage_init(){
    init_nvs();
    init_nvs_partition("dadosNVS");    
    init_spiffs();   
    
    mutex = xSemaphoreCreateMutex();
    //set_default_nvs();
    return ESP_OK;
}

void set_default_nvs(){
    write_nvs_str("dadosNVS","server", "username", "admin");
    write_nvs_str("dadosNVS","server", "password", "admin");    
    write_nvs_int32("dadosNVS", "clock", "autotime_sntp", true);
    write_nvs_str("dadosNVS","clock", "ntp_sever", "a.ntp.br");
    write_nvs_str("dadosNVS","clock", "tz", "GMT-3");
    write_nvs_str("dadosNVS","server", "", "GMT-3");
    write_nvs_int32("dadosNVS", "server", "server", true);    
    write_nvs_str("dadosNVS", "server", "ip", "192.168.1.4");
    write_nvs_str("dadosNVS", "server", "last_check", "");    
    write_nvs_str("dadosNVS", "wifi", "SSID0", "");
    write_nvs_str("dadosNVS", "wifi", "PASS0", "");
    write_nvs_str("dadosNVS", "wifi", "SSID1", "");
    write_nvs_str("dadosNVS", "wifi", "PASS1", "");
    write_nvs_str("dadosNVS", "wifi", "SSID2", "");
    write_nvs_str("dadosNVS", "wifi", "PASS2", "");
    write_nvs_str("dadosNVS", "wifi", "SSID3", "");
    write_nvs_str("dadosNVS", "wifi", "PASS3", "");
    write_nvs_str("dadosNVS", "wifi", "SSID4", "");
    write_nvs_str("dadosNVS", "wifi", "PASS4", "");
    write_nvs_int32("dadosNVS", "timer1", "channel", 0b0000000);
    write_nvs_int32("dadosNVS", "timer1", "week", 0b0000000);
    write_nvs_int32("dadosNVS", "timer1", "hour_st", 9); 
    write_nvs_int32("dadosNVS", "timer1", "min_st", 47); 
    write_nvs_int32("dadosNVS", "timer1", "hour_en", 10); 
    write_nvs_int32("dadosNVS", "timer1", "min_en", 15);  
    write_nvs_int32("dadosNVS", "timer2", "channel", 0b0000000);
    write_nvs_int32("dadosNVS", "timer2", "week", 0b0000111);
    write_nvs_int32("dadosNVS", "timer2", "hour_st", 9); 
    write_nvs_int32("dadosNVS", "timer2", "min_st", 47); 
    write_nvs_int32("dadosNVS", "timer2", "hour_en", 5); 
    write_nvs_int32("dadosNVS", "timer2", "min_en", 12);      
    write_nvs_int32("dadosNVS", "timer3", "channel", 0b0000000);
    write_nvs_int32("dadosNVS", "timer3", "week", 0b0000000);
    write_nvs_int32("dadosNVS", "timer3", "hour_st", 9); 
    write_nvs_int32("dadosNVS", "timer3", "min_st", 47); 
    write_nvs_int32("dadosNVS", "timer3", "hour_en", 10); 
    write_nvs_int32("dadosNVS", "timer3", "min_en", 45);   
    write_nvs_int32("dadosNVS", "timer4", "channel", 0b0000000);
    write_nvs_int32("dadosNVS", "timer4", "week", 0b0000000);
    write_nvs_int32("dadosNVS", "timer4", "hour_st", 9); 
    write_nvs_int32("dadosNVS", "timer4", "min_st", 47); 
    write_nvs_int32("dadosNVS", "timer4", "hour_en", 10); 
    write_nvs_int32("dadosNVS", "timer4", "min_en", 45);       
    write_nvs_int32("dadosNVS", "termostate1", "channel", 0b0000000);
    write_nvs_int32("dadosNVS", "termostate1", "type", 1);
    write_nvs_int32("dadosNVS", "termostate1", "beta", 3500);
    write_nvs_int32("dadosNVS", "termostate1", "mode", 0);
    write_nvs_int32("dadosNVS", "termostate1", "temp", 25);
    write_nvs_int32("dadosNVS", "termostate1", "offset", 1);
    write_nvs_int32("dadosNVS", "termostate2", "channel", 0b0000000);
    write_nvs_int32("dadosNVS", "termostate2", "type", 1);
    write_nvs_int32("dadosNVS", "termostate2", "beta", 3500);
    write_nvs_int32("dadosNVS", "termostate2", "mode", 0);
    write_nvs_int32("dadosNVS", "termostate2", "temp", 25);
    write_nvs_int32("dadosNVS", "termostate2", "offset", 1);
    write_nvs_int32("dadosNVS", "enDigital", "channel", 0b0000000);
    write_nvs_int32("dadosNVS", "led", "mode", 0);
    write_nvs_int32("dadosNVS", "led", "rgb", 0);
    write_nvs_int32("dadosNVS", "led", "time", 1000);
    write_nvs_int32("dadosNVS", "led", "brightness", 100);

}

void shift_w(){
    char ssid_rd []= "SSID ";
    char pass_rd []= "PASS ";
    char ssid_old []= "SSID ";
    char pass_old []= "PASS ";
    
   
    for (int i = 4; i >= 0; i--)
    {   
        ESP_LOGI(TAGn, "i %d", i);
        char ssid[32];
        char pass[64];    
        ssid_rd[4] = i + '0';                    
        pass_rd[4] = i + '0';
        
        if(!i==0){
        int n = i;
        n--;
        ssid_old[4] = n + '0';                    
        pass_old[4] = n + '0';
        
        read_nvs_str("dadosNVS", "wifi", ssid_old, &ssid);               
        n==0? read_nvs_str("dadosNVS", "wifi", "PASS0", &pass):read_nvs_str("dadosNVS", "wifi", pass_old, &pass);
        
        write_nvs_str("dadosNVS", "wifi", ssid_rd, &ssid);
        
        write_nvs_str("dadosNVS", "wifi", pass_rd, &pass);
        
        }else{
            write_nvs_str("dadosNVS", "wifi", "PASS0", "");
            write_nvs_str("dadosNVS", "wifi", "SSID0", "");
        }   
        

    }   
    
    
}

void erase_nvs(){

    esp_err_t ret = nvs_flash_erase;
    if(ret == ESP_ERR_NOT_FOUND){
        ESP_LOGI(TAGn, "A partição NVS não contém nenhuma página vazia.");
        int retry = 0;
        const int retry_count = 3;        
        while(++retry < retry_count || ret == ESP_OK){
        ret = nvs_flash_erase();
        ret = nvs_flash_init();
        }              
    }
    if(ret !=  ESP_OK){
        esp_restart();
    }   
}
void init_nvs(){
    esp_err_t ret = nvs_flash_init();
    if(ret != ESP_OK){
        notify_err(ret);
        erase_nvs();
    }    
    ESP_LOGI(TAGn, "NVS iniciado com sucesso");    
}
void erase_nvs_partition(const char *part){
   esp_err_t ret = nvs_flash_erase_partition(part);
    if(ret == ESP_ERR_NOT_FOUND){
        ESP_LOGE(TAGn, "A partição NVS não contém nenhuma página vazia.");
        int retry = 0;
        const int retry_count = 3;        
        while(++retry < retry_count || ret == ESP_OK){
        ret = nvs_flash_erase_partition(part);
        ret = nvs_flash_init_partition(part);
        }              
    }
    if(ret !=  ESP_OK){
        esp_restart();
    }   
}
void init_nvs_partition(const char *part){
    esp_err_t ret = nvs_flash_init_partition(part);
    if(ret != ESP_OK){
        notify_err(ret);
        erase_nvs_partition(part);
        //default
    }
    ESP_LOGI(TAGn, "Parttição %s NVS iniciado com sucesso", part);
         
}

int32_t read_nvs_int32(const char *partition_name, const char *namespace, const char *key){
    int cont_tent = 0;
    while (cont_tent <3)
    {
        if(xSemaphoreTake(mutex, 200 / portTICK_PERIOD_MS)){
            
            cont_tent = 3;
          xSemaphoreGive(mutex);
        }else{
            ESP_LOGE(TAGn, "Leitor ocupado");
            cont_tent++;
        }
    }
    


    int32_t val;
    esp_err_t ret_nvs;
    nvs_handle_t nvs_handle;
    ret_nvs = nvs_open_from_partition(partition_name, namespace, NVS_READONLY, &nvs_handle);
    notify_err(ret_nvs);
    if(ret_nvs == ESP_OK) {        
        esp_err_t res = nvs_get_i32(nvs_handle, key, &val);
        
        switch (res)
        {
        case ESP_OK:
            
            break;
        case ESP_ERR_NOT_FOUND:
            ESP_LOGE(TAGn, "Valor não encontrado ");
            return-1;
                
        default:
            ESP_LOGE(TAGn, "Erro ao acessar o NVS (%s)", esp_err_to_name(res));
            return -1;
            break;
        }
        nvs_close(nvs_handle);
    }

    return val;
}
esp_err_t read_nvs_str(const char *partition_name, const char *namespace, const char *key, char *str){
    int cont_tent = 0;
    size_t size;
    esp_err_t ret_nvs;
    nvs_handle_t nvs_handle;
    while (cont_tent <3)
    {   
        if(xSemaphoreTake(mutex, 50 / portTICK_PERIOD_MS)){
            ret_nvs = nvs_open_from_partition(partition_name, namespace, NVS_READONLY, &nvs_handle);
            notify_err(ret_nvs);
            if(ret_nvs == ESP_OK) {        
                nvs_get_str(nvs_handle, key, NULL, &size);
                char* str_nvs[size];
                esp_err_t res = nvs_get_str(nvs_handle, key, str_nvs, &size);
                switch (res)
                {
                case ESP_OK:
                    memcpy(str, str_nvs, size);
                    ESP_LOGI(TAGn,"valor string armazenado: %s chave: %s", str, key);
                    cont_tent = 3;
                    xSemaphoreGive(mutex);
                    return res;
                    break;
                case ESP_ERR_NOT_FOUND:
                    ESP_LOGI(TAGn, "Valor não encontrado");
                    xSemaphoreGive(mutex); 
                    return res;
                        
                default:
                    ESP_LOGI(TAGn, "Erro ao acessar o NVS (%s)", esp_err_to_name(res));
                    xSemaphoreGive(mutex);                    
                    return res;
                    break;
                }
                nvs_close(nvs_handle);
            }   
            

            cont_tent = 3;
          xSemaphoreGive(mutex);
          return ret_nvs;
        }else{
            ESP_LOGE(TAGn, "Leitor ocupado f");
            cont_tent++;
        }
    }   
   
    return -1;
}


esp_err_t write_nvs_int32(const char *partition_name, const char *namespace, const char *key, int32_t valor){
    int cont_tent = 0;
    while (cont_tent <3)
    {
        if(xSemaphoreTake(mutex, 200 / portTICK_PERIOD_MS)){
            cont_tent = 3;
          xSemaphoreGive(mutex);
        }else{
            ESP_LOGE(TAGn, "Leitor ocupado");
            cont_tent++;
        }
    }
    
    esp_err_t ret_nvs;
    nvs_handle_t nvs_handle;
    ret_nvs = nvs_open_from_partition(partition_name, namespace, NVS_READWRITE, &nvs_handle);
    notify_err(ret_nvs);
    if(ret_nvs == ESP_OK){       
        esp_err_t res = nvs_set_i32(nvs_handle, key, valor);
        if(res != ESP_OK){
            notify_err(res);
        }else{
            nvs_close(nvs_handle);
            return res;
        }
    }else{
        return ret_nvs;
    }
    ESP_LOGI(TAGn, "Valor gravado: %d\n", valor );
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return ESP_OK;
}
esp_err_t write_nvs_str(const char *partition_name, const char *namespace, const char *key, const char *str){
    int cont_tent = 0;
    while (cont_tent <3)
    {
        if(xSemaphoreTake(mutex, 200 / portTICK_PERIOD_MS)){
            cont_tent = 3;
          xSemaphoreGive(mutex);
        }else{
            ESP_LOGE(TAGn, "Leitor ocupado");
            cont_tent++;
        }
    }
    
    
    nvs_handle_t nvs_handle;
    esp_err_t ret_nvs = nvs_open_from_partition(partition_name, namespace, NVS_READWRITE, &nvs_handle);
    notify_err(ret_nvs);
    if(ret_nvs == ESP_OK){       
        esp_err_t res = nvs_set_str(nvs_handle, key, str);
        if(res != ESP_OK){
            notify_err(res);
            
        }else{
            nvs_close(nvs_handle);
            ESP_LOGI(TAGn, "String save: %s in key: %s", str, key );
            return res;
        }
    }else{
        return ret_nvs;
    }
    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return ESP_OK;
}

void notify_err(esp_err_t ret){
    switch (ret)
    {
    case ESP_ERR_NOT_FOUND:
        ESP_LOGI(TAGn, "Namespace: armazenamento não encontrado");
        break;
    case ESP_ERR_NVS_NO_FREE_PAGES:
        ESP_LOGI(TAGn, "A partição NVS não contém nenhuma página vazia.");
        break;
    case ESP_ERR_NO_MEM:
        ESP_LOGI(TAGn, "A  memória não podea ser alocada para as estruturas internas");
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGI(TAGn, "Namespace: armazenamento não encontrado");
        break;
    case ESP_ERR_NVS_NOT_INITIALIZED:
        ESP_LOGI(TAGn, "O driver de armazenamento não foi inicializado");
        break;
    case ESP_ERR_NVS_PART_NOT_FOUND:
        ESP_LOGI(TAGn, "A partição com o nome especificado não for encontrada");
        break; 
    case ESP_ERR_NVS_INVALID_NAME:
        ESP_LOGI(TAGn, "O nome do namespace não atender às restrições");
        break;
    case ESP_ERR_NVS_INVALID_HANDLE:
        ESP_LOGI(TAGn, "o identificador foi fechado ou é NULL");
        break;
    case ESP_ERR_NVS_READ_ONLY:
        ESP_LOGI(TAGn, "O identificador de armazenamento foi aberto como somente leitura");
        break;
    case ESP_ERR_NVS_REMOVE_FAILED:
        ESP_LOGI(TAGn, "o valor não foi atualizado porque a operação de gravação flash falhou");
        break;
    default:
        break;
    }
}

void nvs_init(){
    // Initialize NVS
    
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

}

/* Function to initialize SPIFFS */
esp_err_t init_spiffs()
{
    ESP_LOGI(TAGs, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,   // This decides the maximum number of files that can be created on the storage
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAGs, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAGs, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAGs, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAGs, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAGs, "Partition size: total: %d, used: %d", total, used);
    init_server("/spiffs");
    return ESP_OK;
}