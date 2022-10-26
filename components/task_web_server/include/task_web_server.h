

#pragma once

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_vfs.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Comprimento máximo que um caminho de arquivo pode ter no armazenamento */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Tamanho maximo pde um arquivo individual. Make sure this
 * Certifique-se de que este valor seja igual ao definido em upload_script.html */
#define MAX_FILE_SIZE   (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"
/* Tamanho do Buffer temporario */
#define SCRATCH_BUFSIZE  8192
#define HEAD "<head><meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta http-equiv=\"X-UA-compatible\" content=\"ie=edge>\"><link rel=\"stylesheet\" type=\"text/css\" href=\"./main.css\"><link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"./favicon.ico\"  /><script defer src=\"./script.js\"></script><title> Central automatica </title></head>"
#define FOOTER "<footer ><div ><h2>Projeto de TCC Automação industrial - Fatec Osasco</h2></div></footer>"
#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)
    
#define URI 1
#define TITLE 2
#define LINKS 3
#define BODY 4

typedef struct{
    const char * uri;
    const char * title;
    const char * links;
    const char * body;
}page_data_t;

typedef struct{
    char *start_section;
}user_session_t;


/**
 * Inicia o servidor web 
 *
 * @param base_path          Caminho para caminho base de dados
 * 
 *
 * @return
 *          - ESP_OK                  if success
 *          - ESP_ERR_INVALID_STATE   if not mounted
 */
esp_err_t start_webserver();
void stop_webserver();
esp_err_t init_server(const char *base_path);
void set_ap_mode(bool mode);


void check_ch_list(cJSON *channel_number,cJSON * type);

/**
 * @brief Key for encryption and decryption
 */
struct file_server_data {    
    char base_path[ESP_VFS_PATH_MAX + 1];/* Caminho base do armazenamento de arquivos */    
    char scratch[SCRATCH_BUFSIZE];/* Buffer para armazenamento temporário durante a transferência de arquivos*/
};

struct stat entry_stat;

#ifdef __cplusplus
}
#endif