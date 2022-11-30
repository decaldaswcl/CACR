/* HTTP File Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include "cJSON.h"
#include <time.h>
#include <sys/time.h>
#include "esp_http_server.h"

#include "esp_err.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "task_web_server.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "task_storage.h"
#include "task_wifi.h"



#include "task_io.h"

static const char *TAG = "file_server";

static struct file_server_data *server_data = NULL;
httpd_handle_t server = NULL;

char login[] = "admin";
char senha[] = "123456";
bool ap_mode = false;
page_data_t data_map[] =
{
    {"/admin/home/*?","home","<script defer src=\"/src/js/home.js\"></script>","/home.html"},
    {"/ap/*?","Assitente de conexão","<script defer src=\"/src/js/redes.js\"></script>","/ap.html"},
    {"/admin/canais/*?", "Canais","<link>","/canais.html"},
    {"/admin/sistema/*?","Sistema","<script defer src=\"/src/js/system.js\"></script>","/system.html"},
    {"/admin/ledC/*?", "Configurações LED","<script defer src=\"/src/js/ledC.js\"></script>","/ledC.html"},
    {"/admin/relogio/*?", "Relogio","<script defer src=\"/src/js/time.js\"></script>","/relogio.html"},
    {"/admin/config/*?", "configuraçõe","<link>","/config.html"},
    {"/admin/channel/*?", "configuração de canais","<script defer src=\"/src/js/channel.js\"></script>","/channel.html"},
    {"/admin/redes/*?", "Redes","<script defer src=\"/src/js/redes.js\"></script>","/redes.html"},
    {"/admin/login/*?", "Login","<link href=\"/src/css/signin.css\" rel=\"stylesheet\">","/login.html"},
    {"/admin/timers/*?","Timers","<script defer src=\"/src/js/timers.js\"></script>","/timers.html"},
    {"/admin/cores/*?","Seleção de cores","<script defer src=\"/src/js/colors.js\"></script>","/colors.html"},    
    {"/admin/termostato/*?","Termostato","<script defer src=\"/src/js/thermostat.js\"></script>","/thermostat.html"},  
    {}
};


void set_ap_mode(bool mode){
    ap_mode = mode;
}

static esp_err_t get_data(page_data_t *data, int type, const char *name, char * buffer ) {

    page_data_t *pg = data;
    int size = 13;
    
    ESP_LOGI(TAG, "Req: %d Uri: %s", type, name);
    if(type==1){
        for (size_t i = 0; i < size; i++)
        {
            
            if(httpd_uri_match_wildcard(pg->uri, name, strlen(name))){
                strcpy(buffer, pg->uri);                 
                return ESP_OK;
            }
            pg++;
        }
        
        return ESP_OK;
    }else if(type==2){        
        for (size_t i = 0; i < size; i++)
        {
            
            if(httpd_uri_match_wildcard(pg->uri, name, strlen(name))){
                strcpy(buffer, pg->title);                 
                return ESP_OK;
            }
            pg++;
        }
        
        return ESP_OK;
    }else if(type== 3){        
        for (size_t i = 0; i < size; i++)
        {
            
            if(httpd_uri_match_wildcard(pg->uri, name, strlen(name))){
                strcpy(buffer, pg->links);                 
                return ESP_OK;
            }
            pg++;
        }        
        return ESP_OK;
    }else if(type== 4){        
        for (size_t i = 0; i < size; i++)
        {
            
            if(httpd_uri_match_wildcard(pg->uri, name, strlen(name))){
                strcpy(buffer, pg->body);                 
                return ESP_OK;
            }
            pg++;
        }        
        return ESP_OK;
    }
    
    return ESP_OK;
}

static esp_err_t donload_get(httpd_req_t *req, const char *filename){

    FILE *fd = NULL;
    

    char * basepath= ((struct file_server_data *)req->user_ctx)->base_path;
    
    size_t basepath_len = strlen(basepath);
    size_t urilen = strlen(filename);
    char * file_name[basepath_len+urilen];
    
    strcpy(file_name,basepath);
    strcat(file_name, filename);
    
    fd = fopen(file_name, "r");

    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filename);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);


    return ESP_OK;
}

void  check_ch_list(cJSON *channel_number,cJSON * type){
     int ch = cJSON_GetNumberValue(channel_number);
     char * str = cJSON_GetStringValue(type);

     if(read_nvs_int32("dadosNVS", "timer1", "channel") == ch){
        write_nvs_int32("dadosNVS", "timer1", "channel", 0);
        set_ch_state(ch, false);
     }else if(read_nvs_int32("dadosNVS", "timer2", "channel") == ch){
         write_nvs_int32("dadosNVS", "timer2", "channel", 0);
         set_ch_state(ch, false);
     }else if(read_nvs_int32("dadosNVS", "timer3", "channel") == ch){
         write_nvs_int32("dadosNVS", "timer3", "channel", 0);
         set_ch_state(ch, false);
     }else if(read_nvs_int32("dadosNVS", "timer4", "channel") == ch){
         write_nvs_int32("dadosNVS", "timer4", "channel", 0);
         set_ch_state(ch, false);
     }else if(read_nvs_int32("dadosNVS", "termostate1", "channel") == ch){
         write_nvs_int32("dadosNVS", "termostate1", "channel", 0);
         set_ch_state(ch, false);
     }else if(read_nvs_int32("dadosNVS", "termostate2", "channel") == ch){
         write_nvs_int32("dadosNVS", "termostate2", "channel", 0);
         set_ch_state(ch, false);
     }

    if(!strcmp(str, "Sem função")){
        write_nvs_int32("dadosNVS", "timer1", "channel", 0);            
    }else if(!strcmp(str, "Timer 1")){
        write_nvs_int32("dadosNVS", "timer1", "channel", ch);
    }else if(!strcmp(str, "Timer 2")){
        write_nvs_int32("dadosNVS", "timer2", "channel", ch); 
    }else if(!strcmp(str, "Timer 3")){
        write_nvs_int32("dadosNVS", "timer3", "channel", ch);
    }else if(!strcmp(str, "Timer 4")){
        write_nvs_int32("dadosNVS", "timer4", "channel", ch);  
    }else if(!strcmp(str, "Termostato 1")){
        write_nvs_int32("dadosNVS", "termostate1", "channel", ch); 
    }else if(!strcmp(str, "Termostato 2")){
        write_nvs_int32("dadosNVS", "termostate2", "channel", ch);
    }else if(!strcmp(str, "En Digital")){
        write_nvs_int32("dadosNVS", "enDigital", "channel", ch);
    }

}

static esp_err_t html_redirect(httpd_req_t *req, const char * uri)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", uri);
    httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};


static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }
    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

static esp_err_t page_view(httpd_req_t *req, const char *uri)
{    
    char *title[50];
    char *links[256];
    char *body[52];
    get_data(data_map, TITLE, uri, &title);
    get_data(data_map, LINKS, uri, &links);
    get_data(data_map, BODY, uri, &body);

    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html>");
    //Head
    httpd_resp_sendstr_chunk(req, "<html lang=\"pt-br\">");    
    httpd_resp_sendstr_chunk(req, "<head>");
    httpd_resp_sendstr_chunk(req, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    httpd_resp_sendstr_chunk(req, "<meta charset=\"UTF-8\">");
    httpd_resp_sendstr_chunk(req, "<title>");
    httpd_resp_sendstr_chunk(req, title);
    httpd_resp_sendstr_chunk(req, "</title>");
    httpd_resp_sendstr_chunk(req, "<link href=\"/src/css/btp.min.css\" rel=\"stylesheet\">");
    if(!strstr(uri, "/admin/login") ){
    httpd_resp_sendstr_chunk(req,"<link href=\"/src/css/dashboard.css\" rel=\"stylesheet\"><link href=\"/src/css/sidebars.css\" rel=\"stylesheet\"> ");
    httpd_resp_sendstr_chunk(req, "<script defer src=\"/src/js/btp.bundle.min.js\"></script><script defer src=\"/src/js/dashboard.js\"></script>");
    }
    httpd_resp_sendstr_chunk(req, links);    
    httpd_resp_sendstr_chunk(req, "<link rel=\"icon\" type=\"image/x-icon\" href=\"/src/img/favicon.ico\">");    
    httpd_resp_sendstr_chunk(req, "</head>");
    if(!strstr(uri, "/admin/login")&& !strstr(uri, "/ap/")){
        donload_get(req,"/header.html");
        if(!req->sess_ctx){
            httpd_resp_sendstr_chunk(req,"<a class=\"nav-link mx-4 fs-3\" href=\"/admin/login\">");
            httpd_resp_sendstr_chunk(req, "entrar");
            httpd_resp_sendstr_chunk(req, "</a></div></div></header>");
        }else{
            httpd_resp_sendstr_chunk(req,"<a class=\"nav-link mx-4 fs-3\" href=\"/admin/logout\">");
            httpd_resp_sendstr_chunk(req, "Sair");
            httpd_resp_sendstr_chunk(req, "</a></div></div></header>");
        }
        
    }
    //Body
    httpd_resp_sendstr_chunk(req, "<body>");
    if(!strstr(uri, "/admin/login") && !strstr(uri, "/ap/")){
        donload_get(req,"/navbar.html");
    }
    

    if(strstr(uri, "/admin/login/error")){
        donload_get(req, "/loginError.html");
        httpd_resp_sendstr_chunk(req, "</body>");
        httpd_resp_sendstr_chunk(req, "</html>");        
        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_OK;  
    }
    donload_get(req, body);

    if(strstr(uri, "/ap") || strstr(uri, "/admin/redes") ){
        httpd_resp_sendstr_chunk(req, "</div></div>");
        httpd_resp_sendstr_chunk(req, "<script> var imp_ssid = document.getElementById('ssid');");       
        httpd_resp_sendstr_chunk(req, "function chose(name){ imp_ssid.value = name;}");
        httpd_resp_sendstr_chunk(req, "</script>");
        httpd_resp_sendstr_chunk(req, "</body>");
        httpd_resp_sendstr_chunk(req, "</html>"); 
        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_OK;  
    }
    
    httpd_resp_sendstr_chunk(req, "</div></div></body>");
    httpd_resp_sendstr_chunk(req, "</html>"); 
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".css")){
    return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".js")){
    return httpd_resp_set_type(req, "text/html");
    }else if (IS_FILE_EXT(filename, ".png")){
    return httpd_resp_set_type(req, "image/png");
    }
    
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}


static esp_err_t index_page_handler(httpd_req_t *req)
{
    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><head>");
    httpd_resp_sendstr_chunk(req, "<meta http-equiv=\"refresh\" content=\"1; url=");
    httpd_resp_sendstr_chunk(req, "http://192.168.1.6");
    httpd_resp_sendstr_chunk(req, "/home\" />");
    httpd_resp_sendstr_chunk(req, "</head></html>");       
    httpd_resp_sendstr_chunk(req, NULL);
    
    return ESP_OK;
}
static esp_err_t ws_handler(httpd_req_t *req)
{

    
    int ch1 = 0;
    int ch2 = 0;
    int ch3 = 0;
    int ch4 = 0;
    int ch5 = 0;
    int ch6 = 0;
    int ch7 = 0;

    // beginning of the ws URI handler
    if (req->method == HTTP_GET) {
        ESP_LOGI (TAG, " Handshake concluído, a nova conexão foi aberta " );        
        return ESP_OK;
    }
    // action after the handshake (read frames)
    char timeZone;
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset (&ws_pkt, 0 , sizeof ( httpd_ws_frame_t ));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }
    
    if(strcmp((char*)ws_pkt.payload,"clock")== 0){
        char *ntp_sever[15];
        read_nvs_str("dadosNVS","clock", "ntp_sever", &ntp_sever); 
        int32_t auto_snpt = read_nvs_int32("dadosNVS","clock", "autotime_sntp");
        read_nvs_str("dadosNVS","clock", "tz", &timeZone);


        time_t now;
        struct tm timeinfo;
        time(&now);
        setenv("TZ", "UTC+3", 1);
        tzset();
        localtime_r(&now, &timeinfo);    
        int i_yr = timeinfo.tm_year + 1900;
        int i_mo = timeinfo.tm_mon + 1;    
        int i_dy= timeinfo.tm_mday;
        int i_hr = timeinfo.tm_hour;
        int i_mn = timeinfo.tm_min;
        char *c_mo[4];
        char *c_dy[4];
        char *c_hr[4] = {};
        char *c_mn[4];
        char *dt_ret [20] = {};
        char *tm_ret[20] = {};
        
        i_mo<10?sprintf(&c_mo, "0%d", i_mo):sprintf(&c_mo, "%d", i_mo);    
        i_dy<10?sprintf(&c_dy, "0%d", i_dy):sprintf(&c_dy, "%d", i_dy);    
        i_hr<10?sprintf(&c_hr, "0%d", i_hr):sprintf(&c_hr, "%d", i_hr);
        i_mn<10?sprintf(&c_mn, "0%d", i_mn):sprintf(&c_mn, "%d", i_mn);
        sprintf(&dt_ret, "%d", i_yr);
        strcat(&dt_ret, "-");
        strcat(&dt_ret, &c_mo);
        strcat(&dt_ret, "-"); 
        strcat(&dt_ret, &c_dy);
        strcat(&tm_ret, &c_hr);
        strcat(&tm_ret, ":"); 
        strcat(&tm_ret, &c_mn);
        cJSON *root, *index;
        root = cJSON_CreateObject();
        
        cJSON_AddItemToObject(root, "clock", index=cJSON_CreateObject());  
        cJSON_AddStringToObject(index, "fuso", &timeZone);
        auto_snpt?cJSON_AddTrueToObject(index, "auto"):cJSON_AddFalseToObject(index, "auto");
        cJSON_AddStringToObject(index, "ntp", &ntp_sever );
        cJSON_AddStringToObject(index, "date", &dt_ret);
        cJSON_AddStringToObject(index, "time", &tm_ret);
        ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
        ws_pkt.payload = (uint8_t*)cJSON_Print(root);
        ws_pkt.len = strlen(cJSON_Print(root));
        httpd_ws_send_frame(req, &ws_pkt);        
        cJSON_Delete(root);
        
        return ESP_OK;
        
        //ESP_LOGI(TAG, "Pacote recebido com mensagem: %s", ws_pkt.payload);
        //ESP_LOGI(TAG, "Pacote typo: %d", ws_pkt.type);

        //httpd_ws_send_frame(req, &ws_pkt);
        //trigger_async_send(req->handle, req);
        
    }else if(strcmp((char*)ws_pkt.payload,"ap")== 0){ 

    start_scan_ap();
    char *str_ret= get_scan();
    

    //ESP_LOGI(TAG, "Cjson %s", str_ret);
    
    ws_pkt.payload = (uint8_t*)str_ret;
    ws_pkt.len = strlen(str_ret);
    httpd_ws_send_frame(req, &ws_pkt);
    }else if(strcmp((char*)ws_pkt.payload,"channels")== 0){
        
       
        for (size_t y = 0; y < 8; y++)
        {
            int val = 0b1111111;
            y==1 ? val = read_nvs_int32("dadosNVS", "timer1", "channel"): NULL;
            y==2 ? val = read_nvs_int32("dadosNVS", "timer2", "channel"): NULL;
            y==3 ? val = read_nvs_int32("dadosNVS", "timer3", "channel"): NULL;
            y==4 ? val = read_nvs_int32("dadosNVS", "timer4", "channel"): NULL;
            y==5 ? val = read_nvs_int32("dadosNVS", "termostate1", "channel"): NULL;
            y==6 ? val = read_nvs_int32("dadosNVS", "termostate2", "channel"): NULL;
            y==7 ? val = read_nvs_int32("dadosNVS", "enDigital", "channel"): NULL;

            for (size_t i = 0; i < 7; i++)
            {
                if(val & (1 << i)){
                
                switch (i)
                {
                case 0:
                    ch1 = y;
                    break; 
                case 1:
                    ch2 = y;
                    break;         
                case 2:
                    ch3 = y;
                    break;   
                case 3:
                    ch4 = y;
                    break;         
                case 4:
                    ch5 = y;
                    break;         
                case 5:
                    ch6 = y;
                    break;         
                case 6:
                    ch7 = y;
                    break;
                
                }
           
            }
        }
        }

        cJSON *root, *index;
        root = cJSON_CreateObject();

        cJSON_AddItemToObject(root, "ch", index=cJSON_CreateObject());
        cJSON_AddNumberToObject(index, "ch1", ch1);
        cJSON_AddNumberToObject(index, "ch1_state", get_ch_state(1));
        cJSON_AddNumberToObject(index, "ch2", ch2);
        cJSON_AddNumberToObject(index, "ch2_state", get_ch_state(2));
        cJSON_AddNumberToObject(index, "ch3", ch3);
        cJSON_AddNumberToObject(index, "ch3_state", get_ch_state(3));
        cJSON_AddNumberToObject(index, "ch4", ch4);
        cJSON_AddNumberToObject(index, "ch4_state", get_ch_state(4));
        cJSON_AddNumberToObject(index, "ch5", ch5);
        cJSON_AddNumberToObject(index, "ch5_state", get_ch_state(5));
        cJSON_AddNumberToObject(index, "ch6", ch6);
        cJSON_AddNumberToObject(index, "ch6_state", get_ch_state(6));
        //cJSON_AddNumberToObject(root, "ch7", ch7);
        ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
        ws_pkt.payload = (uint8_t*)cJSON_Print(root);
        ws_pkt.len = strlen(cJSON_Print(root));
        httpd_ws_send_frame(req, &ws_pkt);
        cJSON_Delete(root);
   
    }else if(strcmp((char*)ws_pkt.payload,"outputs")== 0){
        cJSON *root, *index;
        root = cJSON_CreateObject();        
        cJSON_AddItemToObject(root, "out", index=cJSON_CreateObject());
        cJSON_AddNumberToObject(index, "ch1_state", get_ch_state(1));
        cJSON_AddNumberToObject(index, "ch2_state", get_ch_state(2));
        cJSON_AddNumberToObject(index, "ch3_state", get_ch_state(3));
        cJSON_AddNumberToObject(index, "ch4_state", get_ch_state(4));
        cJSON_AddNumberToObject(index, "ch5_state", get_ch_state(5));
        cJSON_AddNumberToObject(index, "ch6_state", get_ch_state(6));
        ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
        ws_pkt.payload = (uint8_t*)cJSON_Print(root);
        ws_pkt.len = strlen(cJSON_Print(root));
        httpd_ws_send_frame(req, &ws_pkt);
        cJSON_Delete(root);
    }else if(strcmp((char*)ws_pkt.payload,"ledC")== 0){
        cJSON *root;
        root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "mode", read_nvs_int32("dadosNVS", "led", "mode"));
        cJSON_AddNumberToObject(root, "brightness", read_nvs_int32("dadosNVS", "led", "brightness"));
        ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
        ws_pkt.payload = (uint8_t*)cJSON_Print(root);
        ws_pkt.len = strlen(cJSON_Print(root));
        httpd_ws_send_frame(req, &ws_pkt);
        cJSON_Delete(root);
    }else if(strcmp((char*)ws_pkt.payload,"timers")== 0){
        cJSON *root, *timer, *days, *ind;
        char *index1[10];
        char *index2[10];
        char *start[5];
        char *end[5];
        char timer_key[]= "timer ";
        root = cJSON_CreateObject(); 

        
        int wk1 = read_nvs_int32("dadosNVS","timer1", "week");
        int wk2 =read_nvs_int32("dadosNVS", "timer2", "week");
        int wk3 =read_nvs_int32("dadosNVS", "timer3", "week");
        int wk4 =read_nvs_int32("dadosNVS", "timer4", "week");
        
        
        cJSON_AddItemToObject(root, "timers", ind=cJSON_CreateObject());
        for (size_t i = 0; i < 4; i++)
        {
            itoa(i, &index1, 10);
            int n = i;
            n++;
            timer_key[5] = n + '0';
            int hr_st = read_nvs_int32("dadosNVS", timer_key, "hour_st"); 
            int min_st = read_nvs_int32("dadosNVS", timer_key, "min_st"); 
            int hr_en = read_nvs_int32("dadosNVS", timer_key, "hour_en"); 
            int min_en = read_nvs_int32("dadosNVS", timer_key, "min_en"); 
            sprintf(start, "%02d:%02d", hr_st, min_st);
            sprintf(end, "%02d:%02d", hr_en, min_en );
            
            cJSON_AddItemToObject(ind, index1, timer=cJSON_CreateObject()); 
            cJSON_AddItemToObject(timer, "days", days=cJSON_CreateObject());
            for (size_t y = 0; y < 7; y++)
            {   
                itoa(y, &index2,10);
                if(i==0)(wk1 & (1 << y))?cJSON_AddBoolToObject(days, index2 , true):cJSON_AddBoolToObject(days, index2 , false);
                if(i==1)(wk2 & (1 << y))?cJSON_AddBoolToObject(days, index2 , true):cJSON_AddBoolToObject(days, index2 , false);
                if(i==2)(wk3 & (1 << y))?cJSON_AddBoolToObject(days, index2 , true):cJSON_AddBoolToObject(days, index2 , false);
                if(i==3)(wk4 & (1 << y))?cJSON_AddBoolToObject(days, index2 , true):cJSON_AddBoolToObject(days, index2 , false);
            }                    
            cJSON_AddStringToObject(timer, "start", &start);
            cJSON_AddStringToObject(timer, "end", &end);
        }
        
       

        ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
        ws_pkt.payload = (uint8_t*)cJSON_Print(root);
        ws_pkt.len = strlen(cJSON_Print(root));
        httpd_ws_send_frame(req, &ws_pkt);
        
        cJSON_Delete(root);

    }else if(strcmp((char*)ws_pkt.payload,"temp")== 0){
        cJSON *root, *temp, *index;
        root = cJSON_CreateObject(); 
        cJSON_AddItemToObject(root, "temp", index=cJSON_CreateObject());
        cJSON_AddItemToObject(index, "0", temp=cJSON_CreateObject());
        cJSON_AddNumberToObject(temp, "temp", get_read_temp(1));
        cJSON_AddItemToObject(index, "1", temp=cJSON_CreateObject()); 
        cJSON_AddNumberToObject(temp, "temp", get_read_temp(2));
        ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
            ws_pkt.payload = (uint8_t*)cJSON_Print(root);
            ws_pkt.len = strlen(cJSON_Print(root));
            httpd_ws_send_frame(req, &ws_pkt);         
        
        cJSON_Delete(root);  
    }else if(strcmp((char*)ws_pkt.payload,"ctemp")== 0){
      cJSON *root, *temp, *index;
        root = cJSON_CreateObject(); 
        cJSON_AddItemToObject(root, "ctemp", index=cJSON_CreateObject());
        cJSON_AddItemToObject(index, "0", temp=cJSON_CreateObject());
        cJSON_AddNumberToObject(temp, "type", read_nvs_int32("dadosNVS", "termostate1", "type"));
        cJSON_AddNumberToObject(temp, "beta", read_nvs_int32("dadosNVS", "termostate1", "beta"));
        cJSON_AddNumberToObject(temp, "temp_set", read_nvs_int32("dadosNVS", "termostate1", "temp"));
        cJSON_AddNumberToObject(temp, "offset", read_nvs_int32("dadosNVS", "termostate1", "offset"));
        cJSON_AddNumberToObject(temp, "mode", read_nvs_int32("dadosNVS", "termostate1", "mode"));
        cJSON_AddNumberToObject(temp, "temp", get_read_temp(1));
        cJSON_AddItemToObject(index, "1", temp=cJSON_CreateObject()); 
        cJSON_AddNumberToObject(temp, "type", read_nvs_int32("dadosNVS", "termostate2", "type"));
        cJSON_AddNumberToObject(temp, "beta", read_nvs_int32("dadosNVS", "termostate2", "beta"));
        cJSON_AddNumberToObject(temp, "temp_set", read_nvs_int32("dadosNVS", "termostate2", "temp"));
        cJSON_AddNumberToObject(temp, "offset", read_nvs_int32("dadosNVS", "termostate2", "offset"));
        cJSON_AddNumberToObject(temp, "mode", read_nvs_int32("dadosNVS", "termostate2", "mode"));
        cJSON_AddNumberToObject(temp, "temp", get_read_temp(2));
        ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
            ws_pkt.payload = (uint8_t*)cJSON_Print(root);
            ws_pkt.len = strlen(cJSON_Print(root));
            httpd_ws_send_frame(req, &ws_pkt);         
        
        cJSON_Delete(root);  
    }else if(strcmp((char*)ws_pkt.payload,"colors")== 0){
        cJSON *root, *index;
        root = cJSON_CreateObject();    
        cJSON_AddItemToObject(root, "led", index=cJSON_CreateObject());    
        cJSON_AddNumberToObject(index, "color", read_nvs_int32("dadosNVS", "led", "rgb"));
        cJSON_AddNumberToObject(index, "mode", read_nvs_int32("dadosNVS", "led", "mode"));    
        ESP_LOGI(TAG, "Cjson %s", cJSON_Print(root));
        ws_pkt.payload = (uint8_t*)cJSON_Print(root);
        ws_pkt.len = strlen(cJSON_Print(root));
        httpd_ws_send_frame(req, &ws_pkt);       
        
        cJSON_Delete(root);
    }
    
    free(buf);
    
    return ESP_OK;
}


static esp_err_t index_post(httpd_req_t *req){
    esp_err_t ret;
    char ssid_rd[32];
    char pass_rd[32];
    char ssid[32];
    char pass[32];
    
    char filepath[FILE_PATH_MAX];
    char *uri[FILE_PATH_MAX];
    char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri, sizeof(filepath));

    get_data( data_map , URI, filename, &uri);
    if(strlen(uri) == 0){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Não foi possivel processar a requisição");
        return ESP_OK;
    }   

    if(strstr(filename, "/admin/relogio")){
        char*  buf = malloc(req->content_len);        
        
        size_t buf_len = httpd_req_recv(req, buf, req->content_len);
        char *form[] = { "fuso", "ntp", "auto_ntp", "date", "time"};
        
        for (size_t i = 0; i < 4; i++)
        {
            
            char * str_parm = malloc(25);
            httpd_query_key_value(buf, form[i], str_parm, 25 );
            ESP_LOGI(TAG, "%s: %s", form[i] , str_parm);
            if(i == 0){
              ret = write_nvs_str("dadosNVS","clock", "tz", str_parm);  
              
            }else if(i == 1){
                ret = write_nvs_str("dadosNVS","clock", "ntp_sever", str_parm);                
            }
            else if(i == 2){
                if(strcmp(str_parm,"true")==0){
                    ret = write_nvs_int32("dadosNVS", "clock", "autotime_sntp", true);
                        ESP_LOGI(TAG, "Valor: %s", str_parm);
                        httpd_resp_sendstr_chunk(req, NULL);
                    return ESP_OK;
                }else{
                    ret = write_nvs_int32("dadosNVS", "clock", "autotime_sntp", false);
                    ESP_LOGI(TAG, "Valor: %s", str_parm);
                }
                
            }
            
            
            
            free(str_parm);
        }
        
        //httpd_query_key_value(buf, "time", str_parm, 25 );
        //ESP_LOGI(TAG, "/echo handler read %s", str_parm);
        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_OK;
    }else if(strstr(filename, "/admin/ledC")){
        char*  buf = malloc(req->content_len);        
        
        size_t buf_len = httpd_req_recv(req, buf, req->content_len);
        char *form[] = { "mode", "brightness", "", "", ""};
        esp_err_t ret;
        for (size_t i = 0; i < 2; i++)
        {
            char * str_parm = malloc(25);
            httpd_query_key_value(buf, form[i], str_parm, 25 );
            ESP_LOGI(TAG, "%s: %s", form[i] , str_parm);
            if(i == 0){
              ret = write_nvs_int32("dadosNVS", "led", "mode", atoi(str_parm));                           
            }else if(i ==1 ){
                write_nvs_int32("dadosNVS", "led", "brightness", atoi(str_parm));
                led_set_color();
            }

            free(str_parm);
            
        }


        if(ret != ESP_OK){
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Não foi possivel gravar a solicitação");
                return ESP_OK;
            }
        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_OK;
    }else if(strstr(filename, "/admin/channel")){
        char*  buf = malloc(req->content_len);        
        
        size_t buf_len = httpd_req_recv(req, buf, req->content_len);
        char *form[] = { "ch1", "ch2", "ch3", "ch4", "ch5", "ch6", "ch7"};
        esp_err_t ret;
        int tmr1 = 0;
        int tmr2 = 0;
        int tmr3 = 0;
        int tmr4 = 0;
        int termo1 = 0;
        int termo2 = 0;
        int enDig = 0;

        for (size_t i = 0; i < 7; i++)
        {
            char * str_parm = malloc(25);
            httpd_query_key_value(buf, form[i], str_parm, 25 );
            ESP_LOGI(TAG, "%s: %s", form[i] , str_parm);


           if(strstr(str_parm,"1")){
            tmr1 ^= (1 << i);            
           }else if(strstr(str_parm,"2")){
            tmr2 ^= (1 << i);
           }else if(strstr(str_parm,"3")){
            tmr3 ^= (1 << i);
           }else if(strstr(str_parm,"4")){
            tmr4 ^= (1 << i);
           }else if(strstr(str_parm,"5")){
            termo1 ^= (1 << i);
           }else if(strstr(str_parm,"6")){
            termo2 ^= (1 << i);
           }else if(strstr(str_parm,"7")){
            enDig ^= (1 << i);
           }

            free(str_parm);
            
        }

        /*
        if(ret != ESP_OK){
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Não foi possivel gravar a solicitação");
                return ESP_OK;
            }*/

        write_nvs_int32("dadosNVS", "timer1", "channel", tmr1);
        write_nvs_int32("dadosNVS", "timer2", "channel", tmr2);
        write_nvs_int32("dadosNVS", "timer3", "channel", tmr3);
        write_nvs_int32("dadosNVS", "timer4", "channel", tmr4);
        write_nvs_int32("dadosNVS", "termostate1", "channel", termo1);
        write_nvs_int32("dadosNVS", "termostate2", "channel", termo2);
        write_nvs_int32("dadosNVS", "enDigital", "channel", enDig);

        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_OK;
    }else if((strcmp(filename, "/admin/redes") == 0)||(strcmp(filename, "/ap")== 0)){
        char*  buf = malloc(req->content_len);                
        size_t buf_len = httpd_req_recv(req, buf, req->content_len);
        char *form[] = { "ssid", "password", };
        for (size_t i = 0; i < 2; i++)
        {
            char * str_parm = malloc(32);
            httpd_query_key_value(buf, form[i], str_parm, 32 );
            ESP_LOGI(TAG, "%s: %s", form[i] , str_parm);
            if(i==0)memcpy(ssid_rd, str_parm, 32);
            if(i==1)memcpy(pass_rd, str_parm, 32);
            free(str_parm);            
        }        
        for (size_t y = 1; y < 5; y++)
            {
                char ssid_key []= "SSID ";    
                char pass_key []= "PASS ";              
                ssid_key[4] = y + '0';   
                pass_key[4] = y + '0';                           
                read_nvs_str("dadosNVS", "wifi", ssid_key, &ssid);
                read_nvs_str("dadosNVS", "wifi", pass_key, &pass);                
                ESP_LOGI(TAG, "SSID recebido: %s SSID SALVO: %s", ssid_rd, ssid);
                ESP_LOGI(TAG, "PASS recebido: %s PASS SALVO: %s", pass_rd, pass);
                if((strcmp(&ssid_rd, &ssid) == 0) && (strcmp(&pass_rd, &pass) == 0)){
                    ESP_LOGI(TAG, "achou ");
                    write_nvs_str("dadosNVS", "wifi", "SSID0", &ssid_rd);
                    write_nvs_str("dadosNVS", "wifi", "PASS0", &pass_rd);                                        
                    read_nvs_str("dadosNVS", "wifi", "SSID1", &ssid_rd);
                    read_nvs_str("dadosNVS", "wifi", "PASS1", &pass_rd);                    
                    write_nvs_str("dadosNVS", "wifi", ssid_rd, &ssid_rd); 
                    write_nvs_str("dadosNVS", "wifi", pass_rd, &pass_rd);
                    read_nvs_str("dadosNVS", "wifi", "SSID0", &ssid_rd);
                    read_nvs_str("dadosNVS", "wifi", "PASS0", &pass_rd);
                    write_nvs_str("dadosNVS", "wifi", "PASS1", &ssid_rd);
                    write_nvs_str("dadosNVS", "wifi", "SSID1", &pass_rd);
                    y = 5;
                }else if(y == 4){
                    ESP_LOGI(TAG, "não achou ");
                    write_nvs_str("dadosNVS", "wifi", "PASS0", &pass_rd);
                    write_nvs_str("dadosNVS", "wifi", "SSID0", &ssid_rd);
                    y = 5;
                    
                }                
            }
            
        //wifi_new_connect("Willl","12345678");
        httpd_resp_sendstr_chunk(req, NULL);
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
        return ESP_OK;
    }else if(strcmp(filename, "/admin/timers") == 0){
        char*  buf = malloc(req->content_len);                
        size_t buf_len = httpd_req_recv(req, buf, req->content_len);
        char *form[] = { "tm1Week", "tm2Week", "tm3Week", "tm4Week", "tm1start", "tm1end", "tm2start", "tm2end", "tm3start", "tm3end", "tm4start", "tm4end"};
        for (size_t i = 0; i < 12; i++)
        {
            char * str_parm = malloc(32);
            httpd_query_key_value(buf, form[i], str_parm, 32 );
            ESP_LOGI(TAG, "%s: %s", form[i] , str_parm);
            
            for (int y = 1; y < 5; y++)
            {
                char str[2];
                char ret[] = "00";
                itoa(y,str,10);
                if(strstr(form[i], str) != NULL){
                    char timer_key []= "timer ";              
                    timer_key[5] = y + '0';                
                    
                    if(strstr(form[i],"Week") != NULL){
                        
                        int val = strtol(str_parm, NULL, 10 );
                        ESP_LOGI(TAG, " val %s %d", timer_key, val);
                        write_nvs_int32("dadosNVS", timer_key, "week", val);
                    }
                    if(strstr(form[i],"start") != NULL){

                        char ret[] = "00";
                        ret[0] = str_parm[0];
                        ret[1] = str_parm[1];
                        int hr = strtol(ret, NULL, 10 );
                        ret[0] = str_parm[3];
                        ret[1] = str_parm[4];
                        int min = strtol(ret, NULL, 10 );                        
                        write_nvs_int32("dadosNVS", timer_key, "hour_st", hr);
                        write_nvs_int32("dadosNVS", timer_key, "min_st", min); 
                    }
                    if(strstr(form[i],"end") != NULL){
                        char ret[] = "00";
                        ret[0] = str_parm[0];
                        ret[1] = str_parm[1];
                        int hr = strtol(ret, NULL, 10 );
                        ret[0] = str_parm[3];
                        ret[1] = str_parm[4];
                        int min = strtol(ret, NULL, 10 );
                        write_nvs_int32("dadosNVS", timer_key, "hour_en", hr); 
                        write_nvs_int32("dadosNVS", timer_key, "min_en", min);
                    }
                }
            
            }
            
            
            free(str_parm); 
        }
        httpd_resp_sendstr_chunk(req, NULL);        
        return ESP_OK;
    }else if(strcmp(filename, "/admin/cores") == 0){
        char*  buf = malloc(req->content_len);                
        size_t buf_len = httpd_req_recv(req, buf, req->content_len);
        char *form[] = { "color", };
        for (size_t i = 0; i < 1; i++)
        {
            char * str_parm = malloc(32);
            httpd_query_key_value(buf, form[i], str_parm, 32 );
            ESP_LOGI(TAG, "%s: %s", form[i] , str_parm);
                 
            write_nvs_int32("dadosNVS", "led", "rgb", strtol(str_parm, NULL, 10 )); 
            led_set_color();
            free(str_parm); 
        }
        httpd_resp_sendstr_chunk(req, NULL);  
        return ESP_OK;  
        
    }else if(strcmp(filename, "/admin/sistema") == 0){
        set_default_nvs();
        httpd_resp_sendstr_chunk(req, NULL);  
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();      
        return ESP_OK; 
    }else if(strcmp(filename, "/admin/termostato") == 0){
        char*  buf = malloc(req->content_len);                
        size_t buf_len = httpd_req_recv(req, buf, req->content_len);
        char *form[] = { "type1", "type2", "beta1", "beta2", "mode1", "mode2", "offset1", "offset2", "temp1", "temp2" };
        for (size_t i = 0; i < 10; i++)
        {
            char * str_parm = malloc(32);
            httpd_query_key_value(buf, form[i], str_parm, 32 );
            ESP_LOGI(TAG, "%s: %s", form[i] , str_parm);
            for (size_t y = 1; y < 3; y++)
            {
                char str_term []= "termostate ";            
                str_term[10] = y + '0';               
                int val = strtol(str_parm, NULL, 10 );
                if(strstr(form[i], "type1") && y==1)write_nvs_int32("dadosNVS", str_term, "type", val);
                if(strstr(form[i], "type2") && y==2)write_nvs_int32("dadosNVS", str_term, "type", val);
                if(strstr(form[i], "beta1") && y==1)write_nvs_int32("dadosNVS", str_term, "beta", val);
                if(strstr(form[i], "beta2") && y==2)write_nvs_int32("dadosNVS", str_term, "beta", val);
                if(strstr(form[i], "mode1") && y==1)write_nvs_int32("dadosNVS", str_term, "mode", val);
                if(strstr(form[i], "mode2") && y==2)write_nvs_int32("dadosNVS", str_term, "mode", val);
                if(strstr(form[i], "offset1") && y==1)write_nvs_int32("dadosNVS", str_term, "offset", val);
                if(strstr(form[i], "offset2") && y==2)write_nvs_int32("dadosNVS", str_term, "offset", val);
                if(strstr(form[i], "temp1") && y==1)write_nvs_int32("dadosNVS", str_term, "temp", val);
                if(strstr(form[i], "temp2") && y==2)write_nvs_int32("dadosNVS", str_term, "temp", val);
                
            }
                 
            
            
            free(str_parm); 
        }
        httpd_resp_sendstr_chunk(req, NULL);  
        return ESP_OK; 
    }


    char *buf_query = NULL;
    char *buf_hdr = NULL;
    char *param = NULL;
    size_t buf_querylen = 0;
    size_t buf_hdrlen = 0;
    //basic_auth_info_t *basic_auth_info = req->user_ctx;
     buf_hdrlen = httpd_req_get_hdr_value_len(req, "authorization") + 1;
     if (buf_hdrlen > 1) {
        ESP_LOGI(TAG, "Found header " );
        buf_hdr = calloc(1, buf_hdrlen);
        if(httpd_req_get_hdr_value_str(req,"user",buf_hdr,buf_hdrlen) == ESP_OK){
            ESP_LOGI(TAG, "Found: %s ",buf_hdr );
        }
     }
    
    char*  buf = malloc(req->content_len + 1);
    
    ret = httpd_req_recv(req, buf, req->content_len);
    ESP_LOGI(TAG, "/echo handler read %s", buf);
    char *user = calloc(1, req->content_len);
    httpd_query_key_value(buf, "user", user, ret);
    ESP_LOGI(TAG, "user: %s", user);
    
    buf_querylen = httpd_req_get_url_query_len(req) + 1;
    if (buf_querylen > 1) {
        buf_query = calloc(1, buf_querylen);
        if (httpd_req_get_url_query_str(req, buf_query, buf_querylen) == ESP_OK) {
            param = calloc(1, 25);
            httpd_query_key_value(buf_query, "password", param, 25 );
            
            ESP_LOGI(TAG, "Found param: %s", param);
            httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED , "Não permitido");

        } else {
            ESP_LOGE(TAG, "No auth value received");
        }
    }else{
        strcat(filename, "/error" );
        page_view(req, filename ); 
        return ESP_OK;     
    }
    //strcat(filename, "/error" );
    //page_view(req, filename );   
    
    
    return ESP_OK;
}
esp_err_t get_remote_ip(httpd_req_t *req, struct sockaddr_in6* addr_in)
{
    int s = httpd_req_to_sockfd(req);
    socklen_t addrlen = sizeof(*addr_in);
    if (lwip_getpeername(s, (struct sockaddr *)addr_in, &addrlen) != -1) {
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error getting peer's IP/port");
        return ESP_FAIL;
    }
}

bool get_session_state(httpd_req_t *req){
    struct sockaddr_in6 addr_in;
    char *ip = NULL;

    
    if (get_remote_ip(req, &addr_in) == ESP_OK) {
        read_nvs_str("dadosNVS", "server", "ip", &ip);
        if(!strstr(&ip,inet_ntoa(addr_in.sin6_addr.un.u32_addr[3]))){
            //ESP_LOGI(TAG, "Remote IP is %s", inet_ntoa(addr_in.sin6_addr.un.u32_addr[3]));
            return false;
        }
        if(true){

        }
        
    }
    //
    //verifica se seção esta ativa
    



return false;
}

static esp_err_t index_get(httpd_req_t *req)
{
    char *uri[FILE_PATH_MAX];  

    get_data( data_map , URI, req->uri, &uri);
    get_session_state(req);

    if(ap_mode){
        if(!strstr(uri, "/ap") ){
        ESP_LOGI(TAG,"Redrect for: /ap");
        html_redirect(req, "/ap");
        return ESP_OK;
        }
        page_view(req, uri);
        return ESP_OK; 
    }

    if(strlen(uri) == 0){
        ESP_LOGI(TAG,"Redrect for: /admin/home");
        html_redirect(req, "/admin/home");
        return ESP_OK;
    }else if(false){
        ESP_LOGI(TAG, "Sem sessão ativa");
        if(strstr(uri, "/admin/home") ||strstr(uri, "/admin/login") ){
            page_view(req, uri);
            return ESP_OK;  
        }
        html_redirect(req, "/admin/login");  

        return ESP_OK;        
    }
    httpd_resp_set_type(req, "text/html");
    page_view(req, uri);
    

   //httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}
static esp_err_t resources_handler(httpd_req_t *req){

    set_content_type_from_file(req, req->uri);
    

    
    donload_get(req, req->uri);
        
    
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    
    

    
    return ESP_OK;
}


static esp_err_t setap_handler(httpd_req_t *req){
    char ssid[32]= "";
    char pass[64]= "";
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((struct file_server_data*)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);    
    cJSON *cjon_ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *cjon_password = cJSON_GetObjectItem(root, "password");
    
    read_nvs_str("dadosNVS", "wifi", "SSID", &ssid); 
    read_nvs_str("dadosNVS", "wifi", "PASS", &pass);
    write_nvs_str("dadosNVS", "wifi", "oldSSID", &ssid);
    write_nvs_str("dadosNVS", "wifi", "oldPASS", &pass);
    
    write_nvs_str("dadosNVS", "wifi", "SSID", cJSON_GetStringValue(cjon_ssid));
    write_nvs_str("dadosNVS", "wifi", "PASS", cJSON_GetStringValue(cjon_password));
    
    
    cJSON_Delete(root);
    
    
    httpd_resp_sendstr_chunk(req, NULL);
    disconect_wifi();
    return ESP_OK;
}
static esp_err_t setch_handler(httpd_req_t *req){
    char ssid[32]= "";
    char pass[64]= "";
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((struct file_server_data*)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);    
    cJSON *cjson_ch = cJSON_GetObjectItem(root, "channel");
    cJSON *cjson_ch_tp = cJSON_GetObjectItem(root, "type");
    
    
    
    if(!cjson_ch== NULL){
        if(!cjson_ch_tp== NULL)check_ch_list(cjson_ch, cjson_ch_tp);
    }
    
    
      

    cJSON_Delete(root);      
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
static esp_err_t validation_handler(httpd_req_t *req){
    
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((struct file_server_data*)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);    
    cJSON *cjon_login = cJSON_GetObjectItem(root, "login");
    cJSON *cjon_password = cJSON_GetObjectItem(root, "password");
    
    char *retL = cJSON_GetStringValue(cjon_login);
    char *retS = cJSON_GetStringValue(cjon_password);
    
    int compL = strcmp(retS, senha); 
    //int compS = strcmp(ret, login); 

    

    ESP_LOGI("Sever","variavel site: %s", retS );
    ESP_LOGI("Sever","variavel interna: %s", senha );
    ESP_LOGI("Sever","Bool: %d", compL );
    if(!strcmp(retL, login)){
        if(!strcmp(retS, senha)){ 
                httpd_resp_send_chunk(req, NULL, 0);
                cJSON_Delete(root); 
                return ESP_OK;
        }  
    } 
    
    
    cJSON_Delete(root);
    
    
     httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED , "Failed to send file");

    return ESP_OK;
}

void stop_webserver(){
     // Ensure handle is non NULL
     if (server != NULL) {
         // Stop the httpd server
         httpd_stop(server);
     }
     ESP_LOGI(TAG, "Webserver stop");
}

esp_err_t init_server(const char *base_path){

    /* Valida o caminho base para armazenamento de arquivos */
    if (!base_path || strcmp(base_path, "/spiffs") != 0) {
        ESP_LOGE(TAG, "O servidor de arquivos atualmente suporta apenas '/spiffs' como caminho base");
        return ESP_ERR_INVALID_ARG;
    }

    if (server_data) {
        ESP_LOGE(TAG, "Servidor de arquivos já iniciado ");
        return ESP_ERR_INVALID_STATE;
    }

    /* Alocação de memoria para servidor de dados */
    server_data = calloc(1, sizeof(struct file_server_data));
    /* Verifica se a memoria foi alocada*/
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    /* Copia o caminho base para  variavel de dados do servidor */
    strlcpy(server_data->base_path, base_path,
            sizeof(server_data->base_path));

    return ESP_OK;
}

/* função que inicia o webservice*/
esp_err_t start_webserver(){
    ESP_LOGI(TAG, "Webserver start");
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Use a função de correspondência de wildcard de URI para
    * permitir que o mesmo manipulador responda a vários
    * URIs de destino que correspondem ao esquema wildcard */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }  
    
   
    
    httpd_uri_t validation_base = {
        .uri       = "/validation",  // Match all URIs of type /path/to/file
        .method    = HTTP_POST,
        .handler   = validation_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &validation_base);
    httpd_uri_t set_ap = {
        .uri       = "/setap",  // Match all URIs of type /path/to/file
        .method    = HTTP_POST,
        .handler   = setap_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &set_ap);
    httpd_uri_t set_ch = {
        .uri       = "/setch",  // Match all URIs of type /path/to/file
        .method    = HTTP_POST,
        .handler   = setch_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &set_ch);    
    httpd_uri_t ws = {
        .uri       = "/ws",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = ws_handler,
        .user_ctx  = server_data,    // Pass server data as context
        .is_websocket = true,
        .handle_ws_control_frames = true
    };
    httpd_register_uri_handler(server, &ws);   
    httpd_uri_t resources_get = {
        .uri       = "/src/*",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = resources_handler,
        .user_ctx  = server_data    // Pass server data as context
    };       
    httpd_register_uri_handler(server, &resources_get);  
    httpd_uri_t router_manager_get = {
        .uri       = "*",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = index_get,
        .user_ctx  = server_data    // Pass server data as context
    };       
    httpd_register_uri_handler(server, &router_manager_get);  

    httpd_uri_t router_manager_post = {
        .uri       = "*",  // Match all URIs of type /path/to/file
        .method    = HTTP_POST,
        .handler   = index_post,
        .user_ctx  = server_data    // Pass server data as context
    };       
    httpd_register_uri_handler(server, &router_manager_post);  
    

    return ESP_OK;
}


