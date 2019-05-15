#include "esp_types.h"
#include "esp_timer.h"
#include "esp_adc_cal.h"

void recorder_init(uint64_t timer_period, adc_channel_t adc_chanel, adc_atten_t adc_atten, adc_unit_t adc_unit, uint32_t adc_default_v_ref);
void recorder_record();
bool recorder_is_recording();