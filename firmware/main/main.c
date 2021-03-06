#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_types.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_adc_cal.h"
#include "nvs_flash.h"
#include "soc/timer_group_struct.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/timer.h"
#include "driver/periph_ctrl.h"
#include "driver/i2c.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "ssd1306.h"
#include "fonts.h"
#include "xi2c.h"

#define BUTTON_GPIO 4

#define DEFAULT_VREF 1100 // Default Voltage

// WiFI
#define WIFI_SSID "Grandead"
#define WIFI_PASS "my1te2le3ccascad"
#define HOST_IP_ADDR "46.101.116.209"
#define PORT 8086
const int IPV4_GOTIP_BIT = BIT0;
/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group;

// Timer and sound recording
#define ITONE_CYCLE 80000000 // 1 second cycle in 1/16 CPU clock (1/5 µs) for 80 MHz
#define TIMER_DIVIDER 16     //  Hardware timer clock divider
#define CYCLE_DIV 80
#define FREQUNCY 8000     // Hz
#define SAMPLE_DURATION 3 // sec
#define BUFFER_SIZE FREQUNCY *SAMPLE_DURATION * 2
#define CHANNEL ADC_CHANNEL_6 //GPIO34 if ADC1, GPIO14 if ADC2
#define ATTEN ADC_ATTEN_MAX
#define UNIT ADC_UNIT_1
esp_timer_handle_t timer;

// Logic
TaskHandle_t main_task;

esp_adc_cal_characteristics_t *adc_chars;
bool isRecording = false;
bool connectionInProgress = false;
uint8_t buffer[BUFFER_SIZE];
int bufferIndex = 0;

char messageText[128] = "";
// I2C

#define I2C_MASTER_SCL_IO 22        /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 21        /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_1    /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ 100000   /*!< I2C master clock frequency */

#define CHAR_HEIGHT 18
#define CHAR_WIDTH 11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// headers

void readMicLoop();
void printToLCD(char *message);
void sendAudioData();
void startRecordFromMic();
void stopRecordFromMic();
int16_t readMicRaw();
void tcpClientTask(void *pvParameters);

// setup timer

void initTimer()
{
    const esp_timer_create_args_t timerArgs = {
        .callback = &readMicLoop,
        .name = "micLoopTimer"};
    esp_timer_init(timerArgs, timer);
}

// init NVS

void initNVS()
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

// setup WiFI

esp_err_t wifiEventHandler(void *ctx, system_event_t *event)
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
        xEventGroupSetBits(wifi_event_group, IPV4_GOTIP_BIT);
        // ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
        // xEventGroupClearBits(wifi_event_group, IPV6_GOTIP_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void waitForIp()
{
    uint32_t bits = IPV4_GOTIP_BIT;
    printf("Waiting for AP connection...");
    xEventGroupWaitBits(wifi_event_group, bits, false, true, portMAX_DELAY);
    printf("Connected to AP");
}

void initWiFi(void)
{
    wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    printf("Setting WiFi configuration SSID\r\n");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
};

// setup adc

void checkEfuse()
{
    printf("Check if Two Point or Vref are burned into eFuse\r\n");
    // Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        printf("eFuse Two Point: Supported\r\n");
    }
    else
    {
        printf("eFuse Two Point: NOT supported\r\n");
    }

    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        printf("eFuse Vref: Supported\r\n");
    }
    else
    {
        printf("eFuse Vref: NOT supported\r\n");
    }
}

void configureADC()
{
    printf("Configure ADC\r\n");
    if (UNIT == ADC_UNIT_1)
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten((adc1_channel_t)CHANNEL, ATTEN);
    }
    else
    {
        adc2_config_channel_atten((adc2_channel_t)CHANNEL, ATTEN);
    }
}

void printCharValType(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        printf("Characterized using Two Point Value\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        printf("Characterized using eFuse Vref\n");
    }
    else
    {
        printf("Characterized using Default Vref\n");
    }
}

void characterizeADC()
{
    printf("Characterize ADC\r\n");
    adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(UNIT, ATTEN, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    printCharValType(val_type);
}

void initI2C()
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void initLCD()
{
    SSD1306_Init();
}

// setup

void init()
{
    //char m[256] = "message";
    //strncpy(messageText, m, strlen(m));
    printf("-----\r\n");
    printf("Init I2C\r\n");
    initI2C();
    printf("Init LCD\r\n");
    initLCD();
    printToLCD("Loading");
    printf("Init timer\r\n");
    initTimer();
    printf("Init NVS\n\r");
    initNVS();
    printf("Init WiFi\r\n");
    initWiFi();
    waitForIp();
    printf("Configure pinmodes\r\n");
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    printf("Init ADC\r\n");
    checkEfuse();
    configureADC();
    characterizeADC();
}

// loop

void loop()
{
    vTaskDelay(100);
    if (isRecording)
    {
        return;
    }
    if (bufferIndex > 0)
    {
        printToLCD("Sending");
        sendAudioData();
        return;
    }
    // printf("ReadGpio\r\n");
    if (gpio_get_level(BUTTON_GPIO))
    {
        printToLCD("Recording");
        startRecordFromMic();
        return;
    }
    if (strlen(messageText) > 0) {
        printToLCD(messageText);
        return;
    }
    printToLCD("Ready");
    // printf("Mic volume is %d\r\n", readMicRaw()); // for micro settings purpose only
}

void loopTask(void *pvParameters)
{
    printf("Start loop\r\n");
    for (;;)
    {
        loop();
    }
}

// Data received

void sendAudioData()
{
    if (connectionInProgress == false)
    {
        printf("Send audio data\r\n");
        xTaskCreate(tcpClientTask, "tcp_client", 4096, NULL, 5, NULL);
    }
}

void tcpClientTask(void *pvParameters)
{
    connectionInProgress = true;

    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        printf("Unable to create socket\r\n");

        connectionInProgress = false;
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
            printf("Socket unable to connect\r\n");
            // ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        printf("Successfully connected\r\n");

        //ESP_LOGI(TAG, "Successfully connected");

        err = send(sock, buffer, BUFFER_SIZE, 0);

        while (connectionInProgress)
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

                // close(sock);
                printf("Disconnected from %s:%d\r\n", HOST_IP_ADDR, PORT);
                connectionInProgress = false;
                //ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(TAG, "%s", rx_buffer);
            }
            bufferIndex = 0;
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1)
        {
            // ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    connectionInProgress = false;
    vTaskDelete(NULL);
}

// Mic

void readMicLoop()
{
    if (isRecording)
    {
        if (bufferIndex == BUFFER_SIZE)
        {
            isRecording = false;
            stopRecordFromMic();
            return;
        }
        int16_t micValue = readMicRaw();
        buffer[bufferIndex] = (int8_t)(micValue & 0xff);
        buffer[bufferIndex + 1] = (int8_t)(micValue >> 8);
        bufferIndex += 2;
    }
}

void startRecordFromMic()
{
    // printf("Start record at %d milisecond\r\n", millis());
    isRecording = true;
    const esp_timer_create_args_t timerArgs = {
        .callback = &readMicLoop,
        .name = "micLoopTimer"};
    esp_timer_create(&timerArgs, &timer);
    esp_timer_start_periodic(timer, (ITONE_CYCLE / CYCLE_DIV) / FREQUNCY);
    // timer = timerBegin(0, CYCLE_DIV, true);
    // timerAttachInterrupt(timer, &readMicLoop, true);
    // timerAlarmWrite(timer, (ITONE_CYCLE / CYCLE_DIV) / FREQUNCY, true);
    // timerAlarmEnable(timer);
}

void stopRecordFromMic()
{
    esp_timer_stop(timer);
    esp_timer_delete(timer);
    // printf("Stop record at %d milisecond\r\n", millis());
}

int16_t readMicRaw()
{
    int16_t adcValue = 0;
    if (UNIT == ADC_UNIT_1)
    {
        adcValue = adc1_get_raw((adc1_channel_t)CHANNEL);
    }
    else
    {
        int raw;
        adc2_get_raw((adc2_channel_t)CHANNEL, ADC_WIDTH_BIT_12, &raw);
        adcValue = raw;
    }
    return (adcValue - 1555) << 4;
}

// LCD

void printToLCD(char *message)
{
    uint8_t length = strlen(message);
    uint8_t messageWidth = length * CHAR_WIDTH;
    uint8_t messageHeight = CHAR_HEIGHT;
    // printf("message width %d, mesage height %d\r\n", messageWidth, messageHeight);
    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_GotoXY((SCREEN_WIDTH / 2) - (messageWidth / 2), (SCREEN_HEIGHT / 2) - (messageHeight / 2));
    SSD1306_Puts(message, &Font_11x18, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
}

// Utils

void logFile()
{
    for (int i = 0; i < BUFFER_SIZE; i += 2)
    {
        printf("%d, ", buffer[i] + (buffer[i + 1] << 8) - 32768);
    }
    printf("\r\n");
}

// app main

void app_main()
{
    init();
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, &main_task, 1);
}