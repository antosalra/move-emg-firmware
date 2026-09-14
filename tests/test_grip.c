#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "../intent.h"
#include "calibration.h"
#include "envelope.h"
#include "ring_buffer.h"

int test_grip(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 1000; 

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 1000; 


    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                                    0.30f, 0.15f, 100); 

if (result != 0) {
        printf("FAIL: calibration configuration\n");
        return 1;
    }

    if (ctx.state != INTENT_IDLE) {
        printf("FAIL: should start in IDLE\n");
        return 1;
    }

  


    intent_update(&ctx, 500, 100);

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: should enter CLOSING\n");
        return 1;
    }

    for (int i = 0; i <100; i++){
        intent_update(&ctx, 100,100); 
    }

    if (ctx.state != INTENT_HOLDING){
        printf("FAIL: should enter HOLDING\n");
        return 1;
    }

    for (int i = 0; i < 10000; i++) {
    intent_update(&ctx, 100, 100);
    }

    if (ctx.state != INTENT_HOLDING) {
    printf("FAIL: should remain in HOLDING\n");
    return 1;
    }

    printf("PASS: grip persistence\n");

    return 0;

}

int test_full_cycle(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 1000;

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 1000;

    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                                  0.30f, 0.15f, 100);

    if (result != 0) {
        printf("FAIL: calibration configuration\n");
        return 1;
    }

    /* IDLE -> CLOSING */

    intent_update(&ctx, 500, 100);

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: should enter CLOSING\n");
        return 1;
    }

    /* CLOSING -> HOLDING */

    for (int i = 0; i < 100; i++) {
        intent_update(&ctx, 100, 100);
    }

    if (ctx.state != INTENT_HOLDING) {
        printf("FAIL: should enter HOLDING\n");
        return 1;
    }

    /* HOLDING -> OPENING */

    for (int i = 0; i < 100; i++) {
        intent_update(&ctx, 100, 500);
    }

    if (ctx.state != INTENT_OPENING) {
        printf("FAIL: should enter OPENING\n");
        return 1;
    }

    /* OPENING -> IDLE */

    for (int i = 0; i < 100; i++) {
        intent_update(&ctx, 100, 100);
    }

    if (ctx.state != INTENT_IDLE) {
        printf("FAIL: should return to IDLE\n");
        return 1;
    }

    printf("PASS: full cycle\n");

    return 0;
}

int test_hysteresis(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 1000;

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 1000;

    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                                  0.30f, 0.15f, 100);

    if (result != 0) {
        printf("FAIL: calibration configuration\n");
        return 1;
    }

    /* Enter CLOSING */

    intent_update(&ctx, 500, 100);

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: should enter CLOSING\n");
        return 1;
    }

    /*
     * Flexor stays between LOW (235) and HIGH (370).
     * It should remain CLOSING.
     */

    for (int i = 0; i < 1000; i++) {
        int32_t flexor = 250 + (i % 5) * 20;
        intent_update(&ctx, flexor, 100);
    }

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: hysteresis should hold CLOSING\n");
        return 1;
    }

    printf("PASS: hysteresis holds\n");

    return 0;
}

int test_chatter(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 1000;

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 1000;

    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                                  0.30f, 0.15f, 100);

    if (result != 0) {
        printf("FAIL: calibration configuration\n");
        return 1;
    }

    /* Enter CLOSING */

    intent_update(&ctx, 500, 100);

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: should enter CLOSING\n");
        return 1;
    }

    /* Flexor signal oscillates around HIGH threshold */

    for (int i = 0; i < 1000; i++) {

        int32_t flexor;

        if (i % 2 == 0) {
            flexor = 365;
        }
        else {
            flexor = 375;
        }

        intent_update(&ctx, flexor, 100);
    }

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: chatter caused unwanted transition\n");
        return 1;
    }

    printf("PASS: chatter rejection\n");

    return 0;
}

int test_cocontraction(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 1000;

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 1000;

    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                              0.30f, 0.15f, 100);

if (result != 0) {
    printf("FAIL: calibration configuration\n");
    return 1;
}

/* Both channels HIGH */

intent_update(&ctx, 500, 500);

if (ctx.state != INTENT_IDLE) {
    printf("FAIL: co-contraction should keep IDLE\n");
    return 1;
}

printf("PASS: co-contraction\n");

return 0;
}


int test_direct_reversal(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 1000;

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 1000;

    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                                  0.30f, 0.15f, 100);

        if (result != 0) {
        printf("FAIL: calibration configuration\n");
        return 1;
    }

    /* IDLE -> CLOSING */

    intent_update(&ctx, 500, 100);

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: should enter CLOSING\n");
        return 1;
    }

    /* CLOSING -> OPENING */

    for (int i = 0; i < 100; i++) {
        intent_update(&ctx, 100, 500);
    }

    if (ctx.state != INTENT_OPENING) {
        printf("FAIL: should reverse directly to OPENING\n");
        return 1;
    }

    printf("PASS: direct reversal\n");

    return 0;

}

int test_dwell(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 1000;

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 1000;

    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                                  0.30f, 0.15f, 100);

    if (result != 0) {
        printf("FAIL: calibration configuration\n");
        return 1;
    }

    /* IDLE -> CLOSING */

    intent_update(&ctx, 500, 100);

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: should enter CLOSING\n");
        return 1;
    }

    /* Try to reverse before the 100-sample dwell has elapsed */

    for (int i = 0; i < 99; i++) {
        intent_update(&ctx, 100, 500);

        if (ctx.state != INTENT_CLOSING) {
            printf("FAIL: dwell did not block transition\n");
            return 1;
        }
    }

    /* The 100th sample allows the transition */

    intent_update(&ctx, 100, 500);

    if (ctx.state != INTENT_OPENING) {
        printf("FAIL: should enter OPENING after dwell\n");
        return 1;
    }

    printf("PASS: dwell enforcement\n");

    return 0;
}


int test_bad_calibration(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 100;

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 1000;

    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                                  0.30f, 0.15f, 100);

    if (result == 0) {
        printf("FAIL: bad calibration should be rejected\n");
        return 1;
    }

    printf("PASS: bad calibration\n");

    return 0;
}


int test_asymmetric_channels(void)
{
    intent_context ctx;

    intent_init(&ctx);

    calibration flexor_cal;
    flexor_cal.baseline = 100;
    flexor_cal.mcv = 1000;

    calibration extensor_cal;
    extensor_cal.baseline = 100;
    extensor_cal.mcv = 190;

    int result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                                  0.30f, 0.15f, 100);

    if (result != 0) {
        printf("FAIL: calibration configuration\n");
        return 1;
    }

    /* Flexor threshold = 370 */

    intent_update(&ctx, 500, 100);

    if (ctx.state != INTENT_CLOSING) {
        printf("FAIL: flexor should trigger at its own threshold\n");
        return 1;
    }

   intent_init(&ctx);

result = intent_configure(&ctx, &flexor_cal, &extensor_cal,
                          0.30f, 0.15f, 100);

if (result != 0) {
    printf("FAIL: calibration configuration\n");
    return 1;
}

    /* Extensor threshold = 127 */

    intent_update(&ctx, 100, 150);

    if (ctx.state != INTENT_OPENING) {
        printf("FAIL: extensor should trigger at its own threshold\n");
        return 1;
    }

    printf("PASS: asymmetric channels\n");

    return 0;
}

int test_calibration(void)
{
    int32_t relaxed[] = {10, 12, 8, 11, 9};
    int32_t contracted[] = {50, 52, 48, 51, 49};

    calibration cal;

    int result = calibration_calculate(
        &cal,
        relaxed,
        5,
        contracted,
        5
    );

    if (result != 0) {
        printf("FAIL: calibration returned error\n");
        return 1;
    }

    if (cal.baseline != 10) {
        printf("FAIL: baseline = %d\n", cal.baseline);
        return 1;
    }

    if (cal.mcv != 50) {
        printf("FAIL: mcv = %d\n", cal.mcv);
        return 1;
    }

    printf("PASS: calibration\n");
    return 0;
}

int test_calibration_to_intent(void)
{
    int32_t flexor_relaxed[] = {10, 12, 8, 11, 9};
    int32_t flexor_contracted[] = {50, 52, 48, 51, 49};

    int32_t extensor_relaxed[] = {10, 11, 9, 10, 10};
    int32_t extensor_contracted[] = {50, 51, 49, 50, 50};

    calibration flexor_cal;
    calibration extensor_cal;
    intent_context ctx;

    int result;

    result = calibration_calculate(
        &flexor_cal,
        flexor_relaxed,
        5,
        flexor_contracted,
        5
    );

    if (result != 0) {
        printf("FAIL: flexor calibration\n");
        return 1;
    }

    result = calibration_calculate(
        &extensor_cal,
        extensor_relaxed,
        5,
        extensor_contracted,
        5
    );

    if (result != 0) {
        printf("FAIL: extensor calibration\n");
        return 1;
    }

    intent_init(&ctx);

    result = intent_configure(
    &ctx,
    &flexor_cal,
    &extensor_cal,
    INTENT_ENTER_FRACTION,
    INTENT_LEAVE_FRACTION,
    INTENT_MIN_DWELL_SAMPLES
);

    if (result != 0) {
        printf("FAIL: intent configuration\n");
        return 1;
    }

    if (ctx.flexor_hi != 22) {
        printf("FAIL: flexor_hi = %d\n", ctx.flexor_hi);
        return 1;
    }

    if (ctx.flexor_lo != 16) {
        printf("FAIL: flexor_lo = %d\n", ctx.flexor_lo);
        return 1;
    }

    if (ctx.extensor_hi != 22) {
        printf("FAIL: extensor_hi = %d\n", ctx.extensor_hi);
        return 1;
    }

    if (ctx.extensor_lo != 16) {
        printf("FAIL: extensor_lo = %d\n", ctx.extensor_lo);
        return 1;
    }

    printf("PASS: calibration to intent\n");
    return 0;
}

int test_zero_window(void)
{
    struct envelope_t e;

    envelope_init(&e, 1000.0f, 20.0f, 0);

    if (e.window != 1) {
        printf("FAIL: zero window was not corrected\n");
        return 1;
    }

    printf("PASS: zero window\n");
    return 0;
}

int test_ring_buffer_empty_read(void)
{
    struct ring_buffer rb;
    int32_t sample;

    rb_init(&rb);

    if (rb_get(&rb, 0, &sample) == 0) {
        printf("FAIL: empty ring buffer read succeeded\n");
        return 1;
    }

    printf("PASS: empty ring buffer read\n");
    return 0;
}


int test_ring_buffer_wraparound(void)
{
    struct ring_buffer rb;
    int32_t sample;

    rb_init(&rb);

    for (int i = 0; i < WINDOW_SIZE + 10; i++) {
        rb_push(&rb, i);
    }

    if (rb.filled != WINDOW_SIZE) {
        printf("FAIL: ring buffer filled incorrectly\n");
        return 1;
    }

    if (rb_get(&rb, 0, &sample) != 0 || sample != WINDOW_SIZE + 9) {
        printf("FAIL: newest sample incorrect\n");
        return 1;
    }

    if (rb_get(&rb, WINDOW_SIZE - 1, &sample) != 0 || sample != 10) {
        printf("FAIL: oldest sample incorrect\n");
        return 1;
    }

    printf("PASS: ring buffer wraparound\n");
    return 0;
}

int test_envelope_reset(void)
{
    struct envelope_t e;

    envelope_init(&e, 1000.0f, 20.0f, 100);

    envelope_update(&e, 1000);
    envelope_update(&e, 2000);

    if (e.rect_buf.filled == 0) {
        printf("FAIL: envelope buffer did not receive samples\n");
        return 1;
    }

    envelope_reset(&e);

    if (e.rect_buf.filled != 0) {
        printf("FAIL: envelope buffer was not reset\n");
        return 1;
    }

    if (e.prev_input != 0.0f ||
        e.prev_output != 0.0f ||
        e.primed != 0) {
        printf("FAIL: envelope filter state was not reset\n");
        return 1;
    }

    printf("PASS: envelope reset\n");
    return 0;
}


int test_envelope_startup(void)
{
    struct envelope_t e;
    int32_t output;

    envelope_init(&e, 1000.0f, 20.0f, 100);

    output = envelope_update(&e, 1000);

    if (output != 0) {
        printf("FAIL: envelope startup produced transient\n");
        return 1;
    }

    if (e.primed != 1) {
        printf("FAIL: envelope was not primed\n");
        return 1;
    }

    printf("PASS: envelope startup\n");
    return 0;
}

int test_empty_calibration(void)
{
    calibration cal;
    int32_t samples[] = {10, 20, 30};

    if (calibration_calculate(
            &cal,
            samples,
            0,
            samples,
            3) == 0) {
        printf("FAIL: empty relaxed calibration was accepted\n");
        return 1;
    }

    if (calibration_calculate(
            &cal,
            samples,
            3,
            samples,
            0) == 0) {
        printf("FAIL: empty contracted calibration was accepted\n");
        return 1;
    }

    printf("PASS: empty calibration\n");
    return 0;
}

int test_ring_buffer_invalid_age(void)
{
    struct ring_buffer rb;
    int32_t sample;

    rb_init(&rb);

    rb_push(&rb, 100);
    rb_push(&rb, 200);
    rb_push(&rb, 300);

    if (rb_get(&rb, 3, &sample) == 0) {
        printf("FAIL: invalid age was accepted\n");
        return 1;
    }

    printf("PASS: invalid ring buffer age\n");
    return 0;
}

int test_invalid_calibration_order(void)
{
    calibration cal;

    int32_t relaxed[] = {50, 51, 49};
    int32_t contracted[] = {40, 41, 39};

    if (calibration_calculate(
            &cal,
            relaxed,
            3,
            contracted,
            3) == 0) {
        printf("FAIL: contracted signal below baseline was accepted\n");
        return 1;
    }

    printf("PASS: invalid calibration order\n");
    return 0;
}