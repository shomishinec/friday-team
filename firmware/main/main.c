#include "esp_log.h"

#include "app/app_events.h"
#include "app/audio_buffer.h"
#include "app/recorder.h"
// #include "app/lcd.h"
#include "app/wifi.h"

#define BUTTON_GPIO 4
#define ESP_INTR_FLAG_DEFAULT 0 //  have no any idea what is this

static const char *_TAG_MAIN = "MAIN";

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    QueueHandle_t quenue_handler = app_events_get_queue();
    app_event_t evt = {app_event_type_t_button_pressed, NULL};
    xQueueSendFromISR(quenue_handler, &evt, 0);
}

static void init_gpio(void)
{
    ESP_LOGI(_TAG_MAIN, "Init GPIO");
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);           // install gpio isr service
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, NULL); // hook isr handler for specific gpio pin
}

static void loop_task(void *arg)
{
    QueueHandle_t quenue_handler = app_events_get_queue();
    app_event_t *app_evt = malloc(sizeof(app_event_t));
    BaseType_t receiveStatus = errQUEUE_EMPTY;
    ESP_LOGI(_TAG_MAIN, "working");

    for (;;)
    {
        receiveStatus = xQueueReceive(quenue_handler, app_evt, 0);
        if (receiveStatus == pdPASS)
        {
            ESP_LOGI(_TAG_MAIN, "app event received");
            switch (app_evt->type)
            {
            case app_event_type_t_button_pressed:
                recorder_record();
                break;
            case app_event_type_t_audio_data_recordered:
                wifi_send_data((audio_buffer_t *)app_evt->arg);
                break;
            case app_event_type_t_command_received:
                ESP_LOGI(_TAG_MAIN, "received command");
                // ESP_LOGI(_TAG_MAIN, "received command: %s", (char *)app_evt->arg);
                break;
            default:
                ESP_LOGW(_TAG_MAIN, "unexpected app event type");
            }
        }
        else
        {
            ESP_LOGV(_TAG_MAIN, "app event not received");
        }
        // ESP_LOGI(_TAG_MAIN, "Mic volume is %d\r\n", recorder_read()); // for debug purposes only
        vTaskDelay(100 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL); // remove task before task end
}

void app_main(void)
{
    // esp_log_level_set("*", ESP_LOG_WARNING);
    esp_log_level_set("*", ESP_LOG_INFO);
    // lcd_init(I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQNCY, I2C_MASTER_RX_BUFFER_LENGTH, I2C_MASTER_TX_BUFFER_LENGTH, LCD_CHAR_WIDTH, LCD_CHAR_HEIGHT, LCD_WIDTH, LCD_HEIGHT);
    app_events_init();
    recorder_init();
    wifi_init();
    init_gpio();
    xTaskCreatePinnedToCore(loop_task, "loop_task", 8192, NULL, 1, NULL, 0);
}