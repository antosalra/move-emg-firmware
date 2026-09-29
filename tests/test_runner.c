#include <stdio.h>
#include "test_runner.h"

/* Every test returns 0 on pass, non-zero on fail. */
int test_grip(void);
int test_full_cycle(void);
int test_hysteresis(void);
int test_chatter(void);
int test_cocontraction(void);
int test_direct_reversal(void);
int test_dwell(void);
int test_bad_calibration(void);
int test_asymmetric_channels(void);
int test_calibration(void);
int test_calibration_to_intent(void);
int test_zero_window(void);
int test_ring_buffer_empty_read(void);
int test_ring_buffer_wraparound(void);
int test_envelope_reset(void);
int test_envelope_startup(void);
int test_empty_calibration(void);
int test_ring_buffer_invalid_age(void);
int test_invalid_calibration_order(void);

/* A list of functions. Adding a test = adding one line here. */
typedef int (*test_fn)(void);

static const test_fn tests[] = {
    test_grip,
    test_full_cycle,
    test_hysteresis,
    test_chatter,
    test_cocontraction,
    test_direct_reversal,
    test_dwell,
    test_bad_calibration,
    test_asymmetric_channels,
    test_calibration,
    test_calibration_to_intent,
    test_zero_window,
    test_ring_buffer_empty_read,
    test_ring_buffer_wraparound,
    test_envelope_reset,
    test_envelope_startup,
    test_empty_calibration,
    test_ring_buffer_invalid_age,
    test_invalid_calibration_order,
};

int run_all_tests(void)
{
    int total  = (int)(sizeof(tests) / sizeof(tests[0]));
    int failed = 0;

    for (int i = 0; i < total; i++) {
        if (tests[i]() != 0) {
            failed++;
        }
    }

    printf("%d/%d tests passed\r\n", total - failed, total);
    return failed;
}