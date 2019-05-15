#include "audio_buffer.h"

audio_buffer_t *_audio_buffer;

audio_buffer_t *audio_buffer_init(uint16_t buffer_size)
{
    _audio_buffer = malloc(sizeof(audio_buffer_t));
    if (_audio_buffer != NULL)
    {
        uint8_t *buffer_data = malloc(buffer_size * sizeof(uint8_t));
        _audio_buffer->data = buffer_data;
        _audio_buffer->size = buffer_size;
        _audio_buffer->index = 0;
    }
    return _audio_buffer;
};

bool audio_buffer_is_full()
{
    return _audio_buffer->size == _audio_buffer->index;
};

void audio_buffer_set(uint8_t byte)
{
    if (!audio_buffer_is_full())
    {
        _audio_buffer->data[_audio_buffer->index] = byte;
        _audio_buffer->index++;
    }
};

audio_buffer_t *audio_buffer_get()
{
    return _audio_buffer;
};

void audio_buffer_clear()
{
    _audio_buffer->index = 0;
};