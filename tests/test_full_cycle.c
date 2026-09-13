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

    for (int i = 0; i < 100; i++) {
        intent_update(&ctx, 100, 100);
    }

    if (ctx.state != INTENT_HOLDING) {
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

    if (ctx.state != INTENT_IDLE) {
        printf("FAIL: should start in IDLE\n");
        return 1;
    }


    /* IDLE -> CLOSING*/

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


    /*HOLDING -> OPENING */

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