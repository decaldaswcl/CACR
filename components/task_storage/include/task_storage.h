#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif



esp_err_t storage_init();
esp_err_t init_spiffs(void);
esp_err_t set_nvs_data_int( const char *name);

void erase_nvs_partition(const char *part);
void init_nvs_partition(const char *part);
void init_nvs();
void erase_nvs();
void set_default_nvs();
void notify_err(esp_err_t ret);

void shift_w();
esp_err_t  write_nvs_int32(const char *partition_name, const char *namespace, const char *key, int32_t valor);
esp_err_t write_nvs_str(const char *partition_name, const char *namespace, const char *key, const char *str);
int32_t read_nvs_int32(const char *partition_name, const char *namespace, const char *key);
esp_err_t read_nvs_str(const char *partition_name, const char *namespace, const char *key, char *str);
void nvs_init();


#ifdef __cplusplus
}
#endif