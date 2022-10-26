
#pragma once



#ifdef __cplusplus
extern "C" {
#endif

#define LED_OFF_MODE 0
#define LED_READ_MODE 1
#define LED_CONECTED_MODE 2
// Register WiFi functions
void wifi_init(void);



static void print_auth_mode(int authmode);
static void print_cipher_type(int pairwise_cipher, int group_cipher);
void start_scan_ap(void);
char* get_scan();
void disconect_wifi(void);
void start_mdns_service();
void wifi_new_connect(char *w_ssid, char *w_password );

#ifdef __cplusplus
}
#endif


