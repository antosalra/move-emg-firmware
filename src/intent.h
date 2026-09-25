#ifndef INTENT_H
#define INTENT_H

#include <stdint.h>
#include "calibration.h" 

#define INTENT_ENTER_FRACTION 0.30f
#define INTENT_LEAVE_FRACTION 0.15f
#define INTENT_MIN_DWELL_SAMPLES 100

typedef enum {
    INTENT_IDLE = 0,
    INTENT_CLOSING,
    INTENT_HOLDING,
    INTENT_OPENING
} hand_state;

typedef struct {
    hand_state state;

    int32_t flexor_hi;
    int32_t flexor_lo;

    int32_t extensor_hi;
    int32_t extensor_lo;

    uint16_t min_dwell_samples;
    uint16_t samples_in_state;
} intent_context;

void intent_init(intent_context *ctx);

int intent_configure(intent_context *ctx,
                     const calibration *flexor_cal,
                     const calibration *extensor_cal,
                     float enter_fraction,
                     float leave_fraction,
                     uint16_t min_dwell_samples);

hand_state intent_update(intent_context *ctx,
                         int32_t flexor_env,
                         int32_t extensor_env);

#endif