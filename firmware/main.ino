#include <stdio.h>
#include <stdlib.h>

#include <WiFi.h>

#include "Wire.h"

#include "SSD1306.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


#define SERIAL_SPEED 115200
#define BUTTON_GPIO 4

#define DEFAULT_VREF 1100 // Default Voltage

#define WIFI_SSID "Grandead"
#define WIFI_PASS "my1te2le3ccascad"
#define SERVER_HOST "172.20.10.3"
#define SERVER_PORT 3000

#define ITONE_CYCLE 80000000 // 1 second cycle in 1/16 CPU clock (1/5 µs) for 80 MHz
#define CYCLE_DIV 80
#define FREQUNCY 8000     // Hz
#define SAMPLE_DURATION 3 // sec
#define BUFFER_SIZE FREQUNCY *SAMPLE_DURATION * 2

#define CHANNEL ADC_CHANNEL_6 //GPIO34 if ADC1, GPIO14 if ADC2
#define ATTEN ADC_ATTEN_MAX
#define UNIT ADC_UNIT_1

//WiFiClient client;

static esp_adc_cal_characteristics_t *adc_chars;

hw_timer_t *timer = NULL;
bool isRecording = false;
bool connectionInProgress = false;
uint8_t buffer[BUFFER_SIZE];
int bufferIndex = 0;

SSD1306 display(0x3c, 21, 22);


//WiFI
//-------------------------------------
#define HOST_IP_ADDR "46.101.116.209"
#define PORT 8086

const int IPV4_GOTIP_BIT = BIT0;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        /* enable ipv6 */
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, IPV4_GOTIP_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
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

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            {.ssid = WIFI_SSID},
            {.password = WIFI_PASS},
        },
    };
    Serial.println("Setting WiFi configuration SSID ");
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
};

static void tcp_client_task(void *pvParameters)
{
    connectionInProgress = true;

    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    char *payload = "Message from ESP32 ";

    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
                Serial.println("Unable to create socket");

        connectionInProgress = false;
        //ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    Serial.println("Socket created");


    while (1) {
        
        //ESP_LOGI(TAG, "Socket created");

        int err = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0) {
            Serial.println("Socket unable to connect");
           // ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        Serial.println("Successfully connected");

        //ESP_LOGI(TAG, "Successfully connected");

        err = send(sock, buffer, BUFFER_SIZE, 0);

        while (connectionInProgress) {
            if (err < 0) {
                Serial.println("Error occured during sending");

                //ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occured during receiving
            if (len < 0) {
                Serial.println("recv failed: errno");

                //ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                Serial.println("Received %d bytes from");
                Serial.println(rx_buffer);
            
               // close(sock);
                Serial.printf("Disconnected from %s:%d\r\n", HOST_IP_ADDR, PORT);
                connectionInProgress = false;
                //ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(TAG, "%s", rx_buffer);
            }
            bufferIndex = 0;
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    connectionInProgress = false;
    vTaskDelete(NULL);
}

//-------------------------------------

void setup()
{
    Serial.begin(SERIAL_SPEED);
    Serial.println("-----");

    initialise_wifi();
    
    Serial.println("Configure pinmodes");
    pinMode(BUTTON_GPIO, INPUT);
    setupLCD();
    printToLCD("Loading");
    checkEfuse();
    configureADC();
    characterizeADC();
    //connectToFIWI();
}

static void setupLCD()
{
    display.init();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
}

static void checkEfuse()
{
    Serial.println("Check if Two Point or Vref are burned into eFuse");
    // Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        Serial.println("eFuse Two Point: Supported");
    }
    else
    {
        Serial.println("eFuse Two Point: NOT supported");
    }

    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        Serial.println("eFuse Vref: Supported");
    }
    else
    {
        Serial.println("eFuse Vref: NOT supported");
    }
}

static void configureADC()
{
    Serial.println("Configure ADC");
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

static void printCharValType(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        Serial.printf("Characterized using Two Point Value\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        Serial.printf("Characterized using eFuse Vref\n");
    }
    else
    {
        Serial.printf("Characterized using Default Vref\n");
    }
}

static void characterizeADC()
{
    Serial.println("Characterize ADC");
    adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(UNIT, ATTEN, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    printCharValType(val_type);
}

// static void connectToFIWI()
// {
//     Serial.printf("Connecting to %s.\r\n", WIFI_SSID);
//     WiFi.begin(WIFI_SSID, WIFI_PASS);
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(100);
//     }
//     Serial.printf("Connected to %s.\r\n", WIFI_SSID);
// }

void loop()
{
    delay(250);
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
    if (digitalRead(BUTTON_GPIO))
    {
        printToLCD("Recording");
        startRecordFromMic();
        return;
    }
    printToLCD("Ready");
    // Serial.printf("Mic volume is %d\r\n", readMicRaw()); // for micro settings purpose only
}

void startRecordFromMic()
{
    Serial.printf("Start record at %d milisecond\r\n", millis());
    isRecording = true;
    timer = timerBegin(0, CYCLE_DIV, true);
    timerAttachInterrupt(timer, &readMicLoop, true);
    timerAlarmWrite(timer, (ITONE_CYCLE / CYCLE_DIV) / FREQUNCY, true);
    timerAlarmEnable(timer);
}

void stopRecordFromMic()
{
    timerDetachInterrupt(timer);
    timerEnd(timer);
    Serial.printf("Stop record at %d milisecond\r\n", millis());
}

void sendAudioData()
{
    if (connectionInProgress == false) {
        Serial.printf("Send audio data");
        xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
    }
   
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
        buffer[bufferIndex] = (byte)(micValue & 0xff);
        buffer[bufferIndex + 1] = (byte)(micValue >> 8);
        bufferIndex += 2;
    }
}

void logFile()
{
    for (int i = 0; i < BUFFER_SIZE; i += 2)
    {
        Serial.printf("%d, ", buffer[i] + (buffer[i + 1] << 8) - 32768);
    }
    Serial.println("");
}

void printToLCD(String message)
{
    display.clear();
    display.drawString(64, 32, message);
    display.display();
}