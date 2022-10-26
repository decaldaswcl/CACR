idf_component_register(SRCS "task_wifi.c"
                    INCLUDE_DIRS "include"                    
                    REQUIRES task_storage task_web_server json task_io task_system)
