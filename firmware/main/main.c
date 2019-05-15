#include "main.h"

#include "app/audio_buffer.h"
#include "app/recorder.h"
#include "app/lcd.h"
#include "app/wifi.h"

#define I2C_MASTER_NUM I2C_NUM_1      /*!< I2C port number for master dev */
#define I2C_MASTER_SCL_IO 22          /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 21          /*!< gpio number for I2C master data  */
#define I2C_MASTER_FREQNCY 100000     /*!< I2C master clock frequency */
#define I2C_MASTER_RX_BUFFER_LENGTH 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_TX_BUFFER_LENGTH 0 /*!< I2C master do not need buffer */

#define LCD_CHAR_HEIGHT 18
#define LCD_CHAR_WIDTH 11
#define LCD_WIDTH 128
#define LCD_HEIGHT 64

#define CHIP_FREQUNCY 80000000 /*!< 1 second cycle in 1/16 CPU clock (1/5 µs) for 80 MHz */
#define TIMER_DIVIDER 16       /*!< Hardware timer clock divider */
#define CYCLE_DIV 80
#define TIMER_PERIOD (CHIP_FREQUNCY / CYCLE_DIV) / AUDIO_FREQUNCY
#define ADC_CHANNEL ADC_CHANNEL_6
#define ADC_ATTEN ADC_ATTEN_MAX
#define ADC_UNIT ADC_UNIT_1
#define ADC_DEFAULT_V_REF 1100

#define AUDIO_FREQUNCY 8000 /*!< Hz */
#define AUDIO_DURATION 3    /*!< sec */
#define BUFFER_SIZE AUDIO_FREQUNCY *AUDIO_DURATION * 2

#define BUTTON_GPIO 4

TaskHandle_t main_task;
audio_buffer_t *audio_buffer;
char *message;

void init(void)
{
    printf("========================\r\n");
    lcd_init(I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQNCY, I2C_MASTER_RX_BUFFER_LENGTH, I2C_MASTER_TX_BUFFER_LENGTH, LCD_CHAR_WIDTH, LCD_CHAR_HEIGHT, LCD_WIDTH, LCD_HEIGHT);
    lcd_print("Loading");
    message = wifi_init(WIFI_SSID, WIFI_PASSWORD, BIT0);
    audio_buffer = audio_buffer_init(BUFFER_SIZE);
    recorder_init(TIMER_PERIOD, ADC_CHANNEL, ADC_ATTEN, ADC_UNIT, ADC_DEFAULT_V_REF);
    printf("Init GPIO modes\r\n");
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
}

void loop(void)
{
    vTaskDelay(100);
    if (recorder_is_recording())
    {
        return;
    }
    if (audio_buffer_is_full())
    {
        lcd_print("Sending");
        printf("Sending\r\n");
        wifi_send_data(audio_buffer->data, audio_buffer->size, SERVER_IP_ADDRESS, SERVER_PORT);
        return;
    }
    printf("Read GPIO\r\n");
    if (gpio_get_level(BUTTON_GPIO))
    {
        lcd_print("Recording");
        printf("Recording\r\n");
        recorder_record();
        return;
    }
    if (strlen(message) > 0)
    {
        lcd_print(message);
        return;
    }
    lcd_print("Ready");
    printf("Ready\r\n");
    // printf("Mic volume is %d\r\n", readMicRaw()); // for micro settings purpose only
}

void loop_task(void *pvParameters)
{
    printf("Start loop\r\n");
    for (;;)
    {
        loop();
    }
}

void app_main(void)
{
    init();
    xTaskCreatePinnedToCore(loop_task, "loop_task", 8192, NULL, 1, &main_task, 1);
}