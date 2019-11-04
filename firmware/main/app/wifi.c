#include "wifi.h"

#include "config.h"
#include "app_events.h"
#include "audio_buffer.h"

// private properties
static EventGroupHandle_t _wifi_event_group; // FreeRTOS event group to signal when we are connected
static const char *_TAG_WIFI = "WIFI";
static int _s_retry_num = 0;
static bool _connection_in_progress = false;
char _message_text[128];

void _wifi_init_sta(void);
static void _event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void wifi_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(_TAG_WIFI, "mode STA");
    _wifi_init_sta();
}

void _wifi_init_sta(void)
{
    _wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(_TAG_WIFI, "init finished");
    ESP_LOGI(_TAG_WIFI, "connect to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASSWORD);
}

static void _event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (_s_retry_num < 3)
        {
            esp_wifi_connect();
            xEventGroupClearBits(_wifi_event_group, WIFI_CONNECTED_BIT);
            _s_retry_num++;
            ESP_LOGW(_TAG_WIFI, "retry to connect to the AP");
        }
        ESP_LOGE(_TAG_WIFI, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(_TAG_WIFI, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        _s_retry_num = 0;
        xEventGroupSetBits(_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void _wifi_tcp_client_task(void *pvParameters)
{
    _connection_in_progress = true;

    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(SERVER_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
    int err = 0;
    int len = 0;
    int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        ESP_LOGE(_TAG_WIFI, "unable to create socket: errno %d", errno);
        _connection_in_progress = false;
        vTaskDelete(NULL);
    }

    ESP_LOGI(_TAG_WIFI, "socket created");

    for (;;)
    {
        err = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0)
        {
            ESP_LOGE(_TAG_WIFI, "socket unable to connect to %s:%d: errno %d", SERVER_IP_ADDRESS, SERVER_PORT, errno);
            break;
        }
        ESP_LOGI(_TAG_WIFI, "successfully connected to %s:%d", SERVER_IP_ADDRESS, SERVER_PORT);
        err = send(sock, audio_buffer_get()->data, BUFFER_SIZE, 0);
        while (1)
        {
            if (err < 0)
            {
                ESP_LOGE(_TAG_WIFI, "error occured during sending: errno %d", errno);
                break;
            }
            ESP_LOGI(_TAG_WIFI, "data successfully sended");
            len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occured during receiving
            if (len < 0)
            {
                ESP_LOGE(_TAG_WIFI, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                ESP_LOGI(_TAG_WIFI, "received %d bytes from %s:", len, addr_str);
                ESP_LOGI(_TAG_WIFI, "%s ", rx_buffer);
                ESP_LOGI(_TAG_WIFI, "disconnected from %s:%d\r\n", SERVER_IP_ADDRESS, SERVER_PORT);
                // rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                strncpy(_message_text, rx_buffer, 128);
                ESP_LOGI(_TAG_WIFI, "coping from rt_buffer complete");
                app_event_t app_event = {app_event_type_t_command_received, NULL};
                ESP_LOGI(_TAG_WIFI, "app_event inited");
                xQueueSend(app_events_get_queue(), &app_event, 0);
                ESP_LOGI(_TAG_WIFI, "app_event sent");
                break;
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        if (sock != 0)
        {
            ESP_LOGI(_TAG_WIFI, "shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
            break;
        }
    }

    ESP_LOGI(_TAG_WIFI, "sending audio data complete");
    audio_buffer_clear();
    _connection_in_progress = false;
    vTaskDelete(NULL);
}

void wifi_send_data(audio_buffer_t *audio_buffer)
{
    if (_connection_in_progress)
    {
        ESP_LOGW(_TAG_WIFI, "data sending in progress");
        return;
    }
    _connection_in_progress = true;
    ESP_LOGI(_TAG_WIFI, "send audio data");
    xTaskCreatePinnedToCore(_wifi_tcp_client_task, "wifi_tcp_client_task", 4096, NULL, 2, NULL, 0);
}