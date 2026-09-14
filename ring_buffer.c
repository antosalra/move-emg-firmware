#include "ring_buffer.h"

void rb_init(struct ring_buffer *rb)
{
    rb->head = 0; 
    rb->filled = 0;
}

void rb_push(struct ring_buffer *rb, int32_t sample)
{
    rb->data[rb->head] = sample;

    rb->head++;

    if (rb->head >= WINDOW_SIZE) {
        rb->head = 0;
    }

    if (rb->filled < WINDOW_SIZE) {
        rb->filled++;
    }
}


int rb_get(const struct ring_buffer *rb, uint16_t age, int32_t *sample)
{
    if (age >= rb->filled) {
        return 1;
    }

    uint16_t index = (rb->head + WINDOW_SIZE - 1 - age) % WINDOW_SIZE; //pls dont touch

    *sample = rb->data[index];

    return 0;
}
    