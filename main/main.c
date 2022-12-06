#include <stdio.h>
#include "task_wifi.h"
#include "task_storage.h"
#include "task_io.h"


void app_main(void)
{
        
      
    storage_init();     
    init_io();
    wifi_init(); 
      
}
