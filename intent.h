#ifndef INTENT_H
#define INTENT_H

#include <stdint.h>

typedef enum {
    INTENT_IDLE = 0,
    INTENT_CLOSING,
    INTENT_HOLDING,
    INTENT_OPENING
} hand_state; 

typedef struct {
    int32_t baseline;
    int32_t mcv;
} channel_cal;

typedef struct {
    hand_state state;
    
    int32_t flexor_hi;
    int32_t flexor_lo;

    int32_t extensor_hi;
    int32_t extensor_lo; 

    uint16_t min_equal_samples;
} intent_context; 

#endif