#include "recorder.h"
#include "audio_buffer.h"

void adc_check_efuse();
void adc_configure();
void adc_print_char_val_type();
void adc_characterize();
void recorder_stop_record();
void recorder_read_loop();
int16_t recorder_read();

uint64_t _timer_period;
esp_timer_handle_t _timer;

adc1_channel_t _adc_chanel;
adc_atten_t _adc_atten;
adc_unit_t _adc_unit;
uint32_t _adc_default_v_ref;
esp_adc_cal_characteristics_t *_adc_chars;

bool is_recording = false;

void recorder_init(uint64_t timer_period, adc_channel_t adc_chanel, adc_atten_t adc_atten, adc_unit_t adc_unit, uint32_t adc_default_v_ref)
{
    printf("Init recorder\r\n");
    _timer_period = timer_period;
    _adc_chanel = adc_chanel;
    _adc_atten = adc_atten;
    _adc_unit = adc_unit;
    _adc_default_v_ref = adc_default_v_ref;
    printf("Init recorder timer\r\n");
    const esp_timer_create_args_t timerArgs = {
        .callback = &recorder_read_loop,
        .name = "recorder_loop_timer"};
    esp_timer_init(timerArgs, _timer);
    printf("Init recorder ADC\r\n");
    adc_check_efuse();
    adc_configure();
    adc_characterize();
}

void adc_check_efuse()
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

void adc_configure()
{
    printf("Configure ADC\r\n");
    if (_adc_unit == ADC_UNIT_1)
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(_adc_chanel, _adc_atten);
    }
    else
    {
        adc2_config_channel_atten(_adc_chanel, _adc_atten);
    }
}

void adc_print_char_val_type(esp_adc_cal_value_t val_type)
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

void adc_characterize()
{
    printf("Characterize ADC\r\n");
    _adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(_adc_unit, _adc_atten, ADC_WIDTH_BIT_12, _adc_default_v_ref, _adc_chars);
    adc_print_char_val_type(val_type);
}

void recorder_record()
{
    printf("Start recording\r\n");
    is_recording = true;
    const esp_timer_create_args_t timerArgs = {
        .callback = &recorder_read_loop,
        .name = "recorder_loop_timer"};
    esp_timer_create(&timerArgs, &_timer);
    esp_timer_start_periodic(_timer, _timer_period);
}

bool recorder_is_recording()
{
    return is_recording;
}

void recorder_stop_record()
{
    esp_timer_stop(_timer);
    esp_timer_delete(_timer);
    printf("Stop recording\r\n");
}

void recorder_read_loop()
{
    if (is_recording)
    {
        if (audio_buffer_is_full())
        {
            is_recording = false;
            recorder_stop_record();
            return;
        }
        int16_t recorderValue = recorder_read();
        audio_buffer_set((int8_t)(recorderValue & 0xff));
        audio_buffer_set((int8_t)(recorderValue >> 8));
    }
}

int16_t recorder_read()
{
    int16_t adcValue = 0;
    if (_adc_unit == ADC_UNIT_1)
    {
        adcValue = adc1_get_raw(_adc_chanel);
    }
    else
    {
        int raw;
        adc2_get_raw(_adc_chanel, ADC_WIDTH_BIT_12, &raw);
        adcValue = raw;
    }
    return (adcValue - 1555) << 4;
}