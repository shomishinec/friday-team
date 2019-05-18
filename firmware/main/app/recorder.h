#include <stdio.h>

#include "esp_types.h"
#include "esp_adc_cal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/adc.h"
#include "driver/timer.h"

#include "soc/timer_group_struct.h"

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

typedef enum
{
    TIMER_EVENT_TYPE_ACTION = 1,
    TIMER_EVENT_TYPE_WAIT = 2,
    TIMER_EVENT_TYPE_UNKNOWN = 1,
} timer_event_type_t;

typedef struct
{
    timer_event_type_t timer_event_type;
    timer_group_t timer_group;
    timer_idx_t timer_idx;
} timer_event_t;

void recorder_init();
void recorder_record();
bool recorder_is_recording();
uint16_t recorder_read();