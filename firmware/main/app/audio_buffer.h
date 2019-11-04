#include <stdio.h>
#include <stdlib.h>

#include "esp_types.h"

#ifndef DEF_AUDIO_BUFFER_H
#define DEF_AUDIO_BUFFER_H

// -- consts
#define AUDIO_FREQUNCY 16000
#define AUDIO_DURATION 3
#define BUFFER_SIZE AUDIO_FREQUNCY *AUDIO_DURATION * 2

// -- public types
typedef struct
{
    uint8_t *data;
    uint32_t index;
} audio_buffer_t;

#endif // DEF_AUDIO_BUFFER_H

// -- public methods
void audio_buffer_init();
audio_buffer_t *audio_buffer_get();
bool audio_buffer_is_full();
void audio_buffer_set(uint8_t byte);
void audio_buffer_clear();
