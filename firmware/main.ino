#include <stdio.h>
#include <stdlib.h>
#include <WiFi.h>
#include "Wire.h"
#include "SSD1306.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

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

WiFiClient client;

static esp_adc_cal_characteristics_t *adc_chars;

hw_timer_t *timer = NULL;
bool isRecording = false;
uint8_t buffer[BUFFER_SIZE];
int bufferIndex = 0;

SSD1306 display(0x3c, 21, 22);

void setup()
{
    Serial.begin(SERIAL_SPEED);
    Serial.println("-----");

    Serial.println("Configure pinmodes");
    pinMode(BUTTON_GPIO, INPUT);
    setupLCD();
    printToLCD("Loading");
    checkEfuse();
    configureADC();
    characterizeADC();
    connectToFIWI();
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

static void connectToFIWI()
{
    Serial.printf("Connecting to %s.\r\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
    }
    Serial.printf("Connected to %s.\r\n", WIFI_SSID);
}

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
    Serial.printf("Connecting to %s:%d\r\n", SERVER_HOST, SERVER_PORT);
    if (client.connect(SERVER_HOST, SERVER_PORT))
    {
        Serial.printf("Connected to %s:%d\r\n", SERVER_HOST, SERVER_PORT);
        Serial.printf("Sending sample, size = %d\r\n", bufferIndex);
        client.write(buffer, BUFFER_SIZE);
        client.stop();
        Serial.printf("Disconnected from %s:%d\r\n", SERVER_HOST, SERVER_PORT);
        // logFile();
        bufferIndex = 0;
    }
    else
    {
        Serial.printf("Connecting to %s:%d failed\r\n", SERVER_HOST, SERVER_PORT);
        client.stop();
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