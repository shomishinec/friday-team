#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "audio_buffer.h"

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
#define WIFI_CONNECTED_BIT BIT0

void wifi_init();
void wifi_send_data(audio_buffer_t *audio_buffer);