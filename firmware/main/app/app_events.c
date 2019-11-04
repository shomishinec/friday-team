#include "app_events.h"

// -- private properties
QueueHandle_t _app_events_queue = NULL;

// -- body
void app_events_init(void)
{
    _app_events_queue = xQueueCreate(10, sizeof(app_event_t *));
}

QueueHandle_t app_events_get_queue(void)
{
    return _app_events_queue;
};