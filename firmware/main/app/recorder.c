#include "recorder.h"
#include "app_events.h"
#include "audio_buffer.h"

// -- private methods
void _timer_init();
void _timer_isr_handler();
void _timer_handler();
void _adc_check_efuse();
void _adc_configure();
void _adc_print_char_val_type();
void _adc_characterize();
void _recorder_stop_record();
void _recorder_handler();
uint16_t _recorder_read();

// -- private properties
QueueHandle_t _timer_queue;
TaskHandle_t _timer_task_handle;
uint64_t _timer_counter;
esp_adc_cal_characteristics_t *_adc_chars;
uint16_t _adc_value = 0;
#if MULTISAMPLING_COUNT > 1
int64_t _adc_multisampling_value = 0;
uint8_t _multisampling_i = 0;
#endif
#if ADC_UNIT != ADC_UNIT_1
int _adc_raw_out_value;
#endif
bool _is_recording = false;
static const char *_TAG_REC = "RECORDER";

// -- body
void recorder_init()
{
    // printf("Init audio buffer");
    audio_buffer_init();
    // printf("Init recorder timer with period %d\r\n", TIMER_PERIOD);
    _timer_init();
    // printf("Init recorder ADC\r\n");
    _adc_check_efuse();
    _adc_configure();
    _adc_characterize();
}

void _timer_init()
{
    _timer_queue = xQueueCreate(10, sizeof(NULL));
    const timer_config_t timer_config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = true,
        .divider = TIMER_DIVIDER};
    timer_init(TIMER_GROUP, TIMER, &timer_config);
    timer_set_counter_value(TIMER_GROUP, TIMER, 0);
    timer_set_alarm_value(TIMER_GROUP, TIMER, TIMER_PERIOD);
    timer_enable_intr(TIMER_GROUP, TIMER);
    timer_isr_register(TIMER_GROUP, TIMER, _timer_isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL);
}

void _adc_check_efuse()
{
    ESP_LOGI(_TAG_REC, "check if Two Point or vRef are burned into eFuse");
    // Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        ESP_LOGI(_TAG_REC, "eFuse Two Point: supported");
    }
    else
    {
        ESP_LOGI(_TAG_REC, "eFuse Two Point: NOT supported");
    }
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        ESP_LOGI(_TAG_REC, "eFuse vRef: supported");
    }
    else
    {
        ESP_LOGI(_TAG_REC, "eFuse vRef: NOT supported");
    }
}

void _adc_configure()
{
    ESP_LOGI(_TAG_REC, "configure ADC");
#if ADC_UNIT == ADC_UNIT_1
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
#else
    adc2_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
#endif
}

void _adc_print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        ESP_LOGI(_TAG_REC, "characterized using Two Point value");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        ESP_LOGI(_TAG_REC, "characterized using eFuse vRef");
    }
    else
    {
        ESP_LOGI(_TAG_REC, "characterized using default vRef");
    }
}

void _adc_characterize()
{
    ESP_LOGI(_TAG_REC, "characterize ADC");
    _adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT_12, ADC_DEFAULT_V_REF, _adc_chars);
    _adc_print_char_val_type(val_type);
}

void recorder_record()
{
    if (_is_recording)
    {
        ESP_LOGW(_TAG_REC, "recording already started");
        return;
    }
    _is_recording = true;
    ESP_LOGI(_TAG_REC, "recording");
    xTaskCreatePinnedToCore(_timer_handler, "timer_handler", 2048, NULL, 2, &_timer_task_handle, 1);
    timer_start(TIMER_GROUP, TIMER);
}

void IRAM_ATTR _timer_isr_handler()
{
    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[TIMER].update = 1;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if ((intr_status & BIT(TIMER)))
    {
        TIMERG0.int_clr_timers.t0 = 1;
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    TIMERG0.hw_timer[TIMER].config.alarm_en = TIMER_ALARM_EN;

    ++_timer_counter;

    if (_timer_counter > 7999)
    {
        /* Now just send the event data back to the main program task */
        xQueueSendFromISR(_timer_queue, &_timer_counter, 0);
    }
}

void _timer_handler(void *arg)
{
    // TODO
    short any_number_to_received = 0;
    for (;;)
    {
        if (xQueueReceive(_timer_queue, &any_number_to_received, portMAX_DELAY) == pdPASS)
        {
            _recorder_handler();
        }
    }
}

void _recorder_handler()
{
    if (audio_buffer_is_full())
    {
        _recorder_stop_record();
    }
    _adc_value = _recorder_read();
    audio_buffer_set(_adc_value & 0xff);
    audio_buffer_set(_adc_value >> 8);
}

void _recorder_stop_record()
{
    app_event_t app_event = {app_event_type_t_audio_data_recordered, audio_buffer_get()};
    timer_pause(TIMER_GROUP, TIMER);
    ESP_LOGI(_TAG_REC, "timer stop at %lld tick", _timer_counter);
    ESP_LOGI(_TAG_REC, "stop recording");
    _timer_counter = 0;
    _is_recording = false;
    xQueueSend(app_events_get_queue(), &app_event, 0);
    vTaskDelete(_timer_task_handle);
}

uint16_t _recorder_read()
{
#if MULTISAMPLING_COUNT > 1
    _adc_value = 0;
    _multisampling_i = 0;
#endif

#if ADC_UNIT == ADC_UNIT_1
#if MULTISAMPLING_COUNT > 1
    for (; _multisampling_i < MULTISAMPLING_COUNT; _multisampling_i++)
    {
        _adc_value += adc1_get_raw(ADC_CHANNEL);
    }
#else
    _adc_value = adc1_get_raw(ADC_CHANNEL);
#endif
#else
    for (; _multisampling_i < MULTISAMPLING_COUNT; _multisampling_i++)
    {
        adc2_get_raw(ADC_CHANNEL, ADC_WIDTH_BIT_12, &_adc_raw_out_value);
        _adc_value += _adc_raw_out_value;
    }
#if MULTISAMPLING_COUNT > 1
    for (; _multisampling_i < MULTISAMPLING_COUNT; _multisampling_i++)
    {
        adc2_get_raw(ADC_CHANNEL, ADC_WIDTH_BIT_12, &_adc_raw_out_value);
        _adc_value += _adc_raw_out_value;
    }
#else
    adc2_get_raw(ADC_CHANNEL, ADC_WIDTH_BIT_12, &_adc_raw_out_value);
    _adc_value = _adc_raw_out_value;
#endif
#endif

#if MULTISAMPLING_COUNT > 1
    return ((_adc_value / MULTISAMPLING_COUNT) << 2);
#else
    return (_adc_value + 700) << 2;
#endif
}

// for debug purposes only
// uint16_t recorder_read(){
//     return _recorder_read();
// }