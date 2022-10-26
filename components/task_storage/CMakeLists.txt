idf_component_register(SRCS "task_storage.c"
                    INCLUDE_DIRS "include"
                    REQUIRES spiffs nvs_flash task_web_server )
