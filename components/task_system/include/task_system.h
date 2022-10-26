#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_sntp();

void time_sync_notification_cb(struct timeval *tv);

#ifdef __cplusplus
}
#endif