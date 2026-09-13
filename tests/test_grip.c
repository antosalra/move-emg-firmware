#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "../intent.h"
#include "calibration.h"

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