#ifndef ENVELOPE_H
#define ENVELOPE_H
#include <stdint.h>
#include "ring_buffer.h"


struct envelope_t {
    float alpha; //noch nicht init
    float prev_input;
    float prev_output;

   struct ring_buffer rect_buf; 
    uint16_t window;

    uint8_t primed; // 0: prev input no existe, 1: sonst. 

};

void envelope_init(struct envelope_t *e, float sample_rate, float cutoff, uint16_t window);

int32_t envelope_update(struct envelope_t *e, int32_t raw_sample);

void envelope_reset(struct envelope_t *e);


#endif