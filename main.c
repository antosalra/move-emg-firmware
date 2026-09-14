#include <stdio.h>
#include <stdint.h>
#include "ring_buffer.h"
#include "envelope.h"
#include "intent.h"
#include "calibration.h"

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

int main(void){
    
    int result;

    result = test_grip();

    if (result != 0) {
        return 1;
    }

    result = test_full_cycle();

    if (result != 0) {
        return 1;
    }

    

result = test_hysteresis();

if (result != 0) {
    return 1;

}
result = test_chatter();

if (result != 0) {
    return 1;
}

result = test_cocontraction();

if (result != 0) {
    return 1;
}

result = test_direct_reversal();

if (result != 0) {
    return 1;
}

result = test_dwell();

if (result != 0) {
    return 1;
}

result = test_bad_calibration();

if (result != 0) {
    return 1;
}

result = test_asymmetric_channels();

if (result != 0) {
    return 1;
}


result = test_calibration();

if (result != 0) {
    return 1;
}

result = test_calibration_to_intent();

if (result != 0) {
    return 1;
}

if (test_zero_window() != 0) return 1;

if (test_ring_buffer_empty_read() != 0) return 1;

if (test_ring_buffer_wraparound() != 0) return 1;

if (test_envelope_reset() != 0) return 1;

if (test_envelope_startup() != 0) return 1;

if (test_empty_calibration() != 0) return 1;

if (test_ring_buffer_invalid_age() != 0) return 1;

if (test_invalid_calibration_order() != 0) return 1;

printf("All tests passed\n");

return 0;

}