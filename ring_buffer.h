#ifndef RING_BUFFER_H // Claude´s suggestion to avoid defining it twice 
#define RING_BUFFER_H

#include <stdint.h>

#define WINDOW_SIZE 256

struct ring_buffer {
    int32_t data[WINDOW_SIZE];
    uint16_t head;
    uint16_t filled;
};

void rb_init(struct ring_buffer *rb);

void rb_push(struct ring_buffer *rb, int32_t sample);

int rb_get(const struct ring_buffer *rb, uint16_t age, int32_t *sample);


#endif // Claude´s correction

