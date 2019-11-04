#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#ifndef DEF_APP_EVENTS_H
#define DEF_APP_EVENTS_H

// public types
typedef enum
{
    app_event_type_t_button_pressed = 0,
    app_event_type_t_audio_data_recordered,
    app_event_type_t_command_received
} app_event_type_t;
typedef struct
{
    app_event_type_t type;
    void *arg;
} app_event_t;

#endif // DEF_APP_EVENTS_H

// -- public methods
void app_events_init(void);
QueueHandle_t app_events_get_queue(void);
