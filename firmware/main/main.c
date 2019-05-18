#include "app/audio_buffer.h"
#include "app/recorder.h"
// #include "app/lcd.h"
#include "app/wifi.h"

#define BUTTON_GPIO 4

TaskHandle_t main_task;
audio_buffer_t *audio_buffer;
char *message;

void init(void)
{
    printf("========================\r\n");
    // lcd_init(I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQNCY, I2C_MASTER_RX_BUFFER_LENGTH, I2C_MASTER_TX_BUFFER_LENGTH, LCD_CHAR_WIDTH, LCD_CHAR_HEIGHT, LCD_WIDTH, LCD_HEIGHT);
    // lcd_print("Loading");
    message = wifi_init();
    audio_buffer = audio_buffer_init();
    recorder_init();
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
        // lcd_print("Sending");
        printf("Sending\r\n");
        wifi_send_data(audio_buffer->data, BUFFER_SIZE);
        return;
    }
    printf("Read GPIO\r\n");
    if (gpio_get_level(BUTTON_GPIO))
    {
        // lcd_print("Recording");
        printf("Recording\r\n");
        recorder_record();
        return;
    }
    if (strlen(message) > 0)
    {
        // lcd_print(message);
        printf("%s\r\n", message);
        return;
    }
    // lcd_print("Ready");
    printf("Ready\r\n");
    // printf("Mic volume is %d\r\n", recorder_read()); // for micro settings purpose only
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
    xTaskCreatePinnedToCore(loop_task, "loop_task", 8192, NULL, 1, &main_task, 0);
}