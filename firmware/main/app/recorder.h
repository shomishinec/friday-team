#include <stdio.h>

#include "esp_types.h"
#include "esp_adc_cal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "driver/adc.h"
#include "driver/timer.h"

#include "soc/timer_group_struct.h"

// -- consts
#define TIMER_GROUP TIMER_GROUP_0
#define TIMER TIMER_0
#define TIMER_DIVIDER 16                           // Hardware timer clock divider
#define TIMER_SCALE TIMER_BASE_CLK / TIMER_DIVIDER // Convert counter value to seconds
#define TIMER_PERIOD TIMER_SCALE / AUDIO_FREQUNCY
#define ADC_CHANNEL ADC_CHANNEL_6
#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_UNIT ADC_UNIT_1
#define ADC_DEFAULT_V_REF 1100
#define MULTISAMPLING_COUNT 1

// -- public methods
void recorder_init();
void recorder_record();
// uint16_t recorder_read(); // for debug purposes only