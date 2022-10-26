#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "task_system.h"

static const char *TAG = "System";

void init_system(){

}
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}
void init_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "a.ntp.br");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    sntp_init();
}
