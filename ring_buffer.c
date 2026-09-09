#include "ring_buffer.h"

void rb_init(struct ring_buffer *rb)
{
    rb->head = 0; 
    rb->filled = 0;
}

void rb_push(struct ring_buffer *rb, int32_t sample){
    rb->data[rb->head] = sample;
    rb->head = (rb->head +1) % WINDOW_SIZE; 

    if(rb->head == 0){ 
    //if(rb->head == WINDOW_SIZE - 1 ){
    rb->filled = 1; 
    }
}

int32_t rb_get(const struct ring_buffer *rb, uint16_t age){
    uint16_t index = (rb->head + WINDOW_SIZE - 1 - age) % WINDOW_SIZE; //pls dont touch

    return rb->data[index]; 

}