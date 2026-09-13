#include "intent.h"

void intent_init(intent_context *ctx){
    ctx->state = INTENT_IDLE;

    ctx->flexor_hi = 0;
    ctx->flexor_lo = 0;

    ctx->extensor_hi = 0;
    ctx->extensor_lo = 0;

    ctx->min_dwell_samples = 0;
    ctx->samples_in_state = 0;
}

int intent_configure(intent_context *ctx,
                     const calibration *flexor_cal,
                     const calibration *extensor_cal,
                     float enter_fraction,
                     float leave_fraction,
                     uint16_t min_dwell_samples){
                        
    if (extensor_cal->mcv <= extensor_cal->baseline ||
        flexor_cal->mcv <= flexor_cal->baseline) {
            return 1;
        }
    
    ctx->flexor_hi = (int32_t)(flexor_cal->baseline + enter_fraction *
    (flexor_cal->mcv - flexor_cal->baseline));

    ctx->flexor_lo = (int32_t)(flexor_cal->baseline + leave_fraction *
    (flexor_cal->mcv - flexor_cal->baseline));
    
    ctx->extensor_hi = (int32_t)(extensor_cal->baseline
    + enter_fraction * (extensor_cal->mcv - extensor_cal->baseline));

    ctx->extensor_lo = (int32_t)(extensor_cal->baseline
    + leave_fraction * (extensor_cal->mcv - extensor_cal->baseline));

    ctx->min_dwell_samples = min_dwell_samples;
    ctx->samples_in_state = 0;

    return 0;
}

hand_state intent_update(intent_context *ctx,
                         int32_t flexor_env,
                         int32_t extensor_env)

// Flexor HIGH + extensor not HIGH → CLOSING
// Extensor HIGH + flexor not HIGH → OPENING
// Both HIGH → stay IDLE
// Otherwise → stay IDLE
{
    uint8_t flexor_high = flexor_env > ctx->flexor_hi;
    uint8_t flexor_low = flexor_env < ctx->flexor_lo;

    uint8_t extensor_high = extensor_env > ctx->extensor_hi;
    uint8_t extensor_low = extensor_env < ctx->extensor_lo;

   if (ctx->samples_in_state < ctx->min_dwell_samples) {
    ctx->samples_in_state++;
}

if (ctx->state == INTENT_IDLE) {

        if (flexor_high && !extensor_high) {
            ctx->state = INTENT_CLOSING;
            ctx->samples_in_state = 0;
        }
        else if (extensor_high && !flexor_high) {
            ctx->state = INTENT_OPENING;
            ctx->samples_in_state = 0;
        }
    }

    else if (ctx->state == INTENT_CLOSING) {

        if (ctx->samples_in_state >= ctx->min_dwell_samples) {

            if (flexor_low && !extensor_high) {
                ctx->state = INTENT_HOLDING;
                ctx->samples_in_state = 0;
            }
            else if (flexor_low && extensor_high) {
                ctx->state = INTENT_OPENING;
                ctx->samples_in_state = 0;
            }
        }
    }

    else if (ctx->state == INTENT_HOLDING) {



        if (ctx->samples_in_state >= ctx->min_dwell_samples) {

            if (extensor_high && !flexor_high) {
                ctx->state = INTENT_OPENING;
                ctx->samples_in_state = 0;
            }
            else if (flexor_high && !extensor_high) {
                ctx->state = INTENT_CLOSING;
                ctx->samples_in_state = 0;
            }
        }
    }

    else if (ctx->state == INTENT_OPENING) {

        if (ctx->samples_in_state >= ctx->min_dwell_samples) {

            if (extensor_low && !flexor_high) {
                ctx->state = INTENT_IDLE;
                ctx->samples_in_state = 0;
            }
            else if (extensor_low && flexor_high) {
                ctx->state = INTENT_CLOSING;
                ctx->samples_in_state = 0;
            }
        }
    }

    return ctx->state;
}