
#include "wifi.h"
#include "config.h"
// TODO hack
#include "audio_buffer.h"

EventGroupHandle_t _wifi_event_group_handle;
bool _connection_in_progress = false;
uint8_t *_buffer;
uint32_t _buffer_size;
char messageText[128];

void nvs_init();
void wifi_connect();
void wifi_wait_for_ip();
esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
void wifi_tcp_client_task(void *pvParameters);

char *wifi_init()
{
    printf("Init nvs\n\r");
    nvs_init();
    printf("Init wifi\r\n");
    wifi_connect();
    wifi_wait_for_ip();
    return messageText;
}

void nvs_init()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
}

void wifi_connect()
{
    _wifi_event_group_handle = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    wifi_config_t wifi_config = {
        .sta = {
            // TODO hach
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    printf("Setting wifi configuration ssid\r\n");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    printf("Connect to WIFI with SSID %s and PASSWORD %s\n\r", WIFI_SSID, WIFI_PASSWORD);
    ESP_ERROR_CHECK(esp_wifi_start());
};

void wifi_wait_for_ip()
{

    printf("Waiting for access point connection...");
    xEventGroupWaitBits(_wifi_event_group_handle, BIT0, false, true, portMAX_DELAY);
    printf("Connected to access point");
}

esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        // ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        /* enable ipv6 */
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(_wifi_event_group_handle, BIT0);
        // ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(_wifi_event_group_handle, BIT0);
        // xEventGroupClearBits(wifi_event_group, IPV6_GOTIP_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_tcp_client_task(void *pvParameters)
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

    int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        printf("Unable to create socket\r\n");

        _connection_in_progress = false;
        //ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    printf("Socket created\r\n");

    while (1)
    {

        //ESP_LOGI(TAG, "Socket created");

        int err = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0)
        {
            printf("Socket unable to connect to %s:%d\n\rError number is %d\r\n", SERVER_IP_ADDRESS, SERVER_PORT, err);
            // ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        printf("Successfully connected\r\n");

        //ESP_LOGI(TAG, "Successfully connected");

        err = send(sock, _buffer, _buffer_size, 0);

        while (_connection_in_progress)
        {
            if (err < 0)
            {
                printf("Error occured during sending\r\n");

                //ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occured during receiving
            if (len < 0)
            {
                printf("recv failed: errno\r\n");

                //ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                printf("Received %d bytes from\r\n", len);
                printf("=========payload========\r\n");
                printf(rx_buffer);
                printf("\r\n");
                printf("========================\r\n");
                strncpy(messageText, rx_buffer, 10);
                printf(messageText);
                printf("Disconnected from %s:%d\r\n", SERVER_IP_ADDRESS, SERVER_PORT);
                _connection_in_progress = false;
                //ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(TAG, "%s", rx_buffer);
            }
            audio_buffer_clear();
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1)
        {
            // ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    _connection_in_progress = false;
    vTaskDelete(NULL);
}

void wifi_send_data(uint8_t *buffer, uint32_t buffer_size)
{
    if (_connection_in_progress == false)
    {
        _connection_in_progress = true;
        _buffer = buffer;
        _buffer_size = buffer_size;
        printf("Send audio data\r\n");
        xTaskCreate(wifi_tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
    }
}
