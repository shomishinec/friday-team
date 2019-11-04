#include "audio_buffer.h"

// -- private properties
audio_buffer_t *_audio_buffer;

// -- body
void audio_buffer_init()
{
    _audio_buffer = malloc(sizeof(audio_buffer_t));
    if (_audio_buffer != NULL)
    {
        uint8_t *buffer_data = malloc(BUFFER_SIZE * sizeof(uint8_t));
        _audio_buffer->data = buffer_data;
        _audio_buffer->index = 0;
    }
};

audio_buffer_t *audio_buffer_get()
{
    return _audio_buffer;
};

bool audio_buffer_is_full()
{
    return _audio_buffer->index == BUFFER_SIZE;
};

void audio_buffer_set(uint8_t byte)
{
    if (!audio_buffer_is_full())
    {
        _audio_buffer->data[_audio_buffer->index] = byte;
        _audio_buffer->index++;
    }
};

void audio_buffer_clear()
{
    _audio_buffer->index = 0;
};
