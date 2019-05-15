#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

char *wifi_init(char *wifi_ssid, char *wifi_password, uint32_t wifi_ipv4_got_ip_bit);
void wifi_send_data(uint8_t *buffer, uint16_t buffer_size, char *server_ip_address, uint16_t server_port);