#include <stdio.h>
#include <stdlib.h>

#include "esp_types.h"

typedef struct
{
    uint8_t *data;
    uint16_t size;
    uint16_t index;
} audio_buffer_t;

audio_buffer_t *audio_buffer_init(uint16_t bufferSize);
bool audio_buffer_is_full();
void audio_buffer_set(uint8_t byte);
audio_buffer_t *audio_buffer_get();
void audio_buffer_clear();