#include "recorder.h"
#include "audio_buffer.h"

void _timer_init();
void _timer_isr_handler();
void _timer_handler();
void _adc_check_efuse();
void _adc_configure();
void _adc_print_char_val_type();
void _adc_characterize();
void _recorder_stop_record();
void _recorder_handler();
// int16_t _recorder_read();

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

void recorder_init()
{
    printf("Init recorder\r\n");
    printf("Init recorder timer with period %d\r\n", TIMER_PERIOD);
    _timer_init();
    printf("Init recorder ADC\r\n");
    _adc_check_efuse();
    _adc_configure();
    _adc_characterize();
    // adc_power_on();
}

void _timer_init()
{
    _timer_queue = xQueueCreate(10, sizeof(timer_event_t));
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
    printf("Check if Two Point or vRef are burned into eFuse\r\n");
    // Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        printf("eFuse Two Point: supported\r\n");
    }
    else
    {
        printf("eFuse Two Point: NOT supported\r\n");
    }
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        printf("eFuse vRef: supported\r\n");
    }
    else
    {
        printf("eFuse vRef: NOT supported\r\n");
    }
}

void _adc_configure()
{
    printf("Configure ADC\r\n");
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
        printf("Characterized using Two Point value\r\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        printf("Characterized using eFuse vRef\r\n");
    }
    else
    {
        printf("Characterized using default vRef\r\n");
    }
}

void _adc_characterize()
{
    printf("Characterize ADC\r\n");
    _adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT_12, ADC_DEFAULT_V_REF, _adc_chars);
    _adc_print_char_val_type(val_type);
}

void recorder_record()
{
    _is_recording = true;
    xTaskCreatePinnedToCore(_timer_handler, "_timer_handler", 2048, NULL, 5, &_timer_task_handle, 1);
    timer_start(TIMER_GROUP, TIMER);
    printf("Start recording\r\n");
}

bool recorder_is_recording()
{
    return _is_recording;
}

void _recorder_stop_record()
{
    _is_recording = false;
    timer_pause(TIMER_GROUP, TIMER);
    printf("Timer stop at %lld tick\r\n", _timer_counter);
    printf("Stop recording\r\n");
    _timer_counter = 0;
    vTaskDelete(_timer_task_handle);
}

void IRAM_ATTR _timer_isr_handler()
{
    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[TIMER].update = 1;

    /* Prepare basic event data
       that will be then sent back to the main program task */
    timer_event_t evt;
    evt.timer_event_type = TIMER_EVENT_TYPE_ACTION;
    evt.timer_group = TIMER_GROUP;
    evt.timer_idx = TIMER;

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
        xQueueSendFromISR(_timer_queue, &evt, NULL);
    }
}

void _timer_handler()
{
    for (;;)
    {
        timer_event_t evt;
        xQueueReceive(_timer_queue, &evt, portMAX_DELAY);
        if (evt.timer_event_type == TIMER_EVENT_TYPE_ACTION)
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
    _adc_value = recorder_read();
    audio_buffer_set(_adc_value & 0xff);
    audio_buffer_set(_adc_value >> 8);
}

uint16_t recorder_read()
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