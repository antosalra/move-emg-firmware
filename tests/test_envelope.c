#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "envelope.h" 


 /* Test 1: DC rejection */
int test_dc_rejection(void){
    struct envelope_t e;

    envelope_init(&e, 1000.0f, 20.0f, 100); 
    
    envelope_update(&e, 0);

    for (int i = 0; i < 1000; i++) {
        int32_t output = envelope_update(&e, 1000);

        /* after 500 ms DC response shoul be 0 */
        if (i > 500 && output > 10) {
    printf("DC failure: i = %d, output = %d\n", i, output);
    return 0;
        }
    }

    return 1;
}

/*Test 2: Attenuation */
int test_1hz_attenuation(void)
{
    struct envelope_t e;

    envelope_init(&e, 1000.0f, 20.0f, 100);

    for (int i = 0; i < 2000; i++) {
        float t = (float)i / 1000.0f;

        int32_t input = (int32_t)(1000.0f *
                                  sinf(2.0f * 3.14159265f * 1.0f * t));

        int32_t output = envelope_update(&e, input);

        /* High pass -> signal should be atenuated */
        if (i > 1000 && output > 100) {
            return 0;
        }
    }

    return 1;
}


/* Test 3:  50hz */
int test_50hz_response(void)
{
    struct envelope_t e;

    envelope_init(&e, 1000.0f, 20.0f, 100);

    for (int i = 0; i < 2000; i++) {
        float t = (float)i / 1000.0f;

        int32_t input = (int32_t)(1000.0f *
                                  sinf(2.0f * 3.14159265f * 50.0f * t));

        int32_t output = envelope_update(&e, input);

        /* Ignore the initial transient */
        if (i > 1000 && output < 400) {
            return 0;
        }
    }

    return 1;
}


/* Test 4: Variing amplitude*/
int test_amplitude_tracking(void)
{
    struct envelope_t e;

    envelope_init(&e, 1000.0f, 20.0f, 100);

    int32_t max_low = 0;
    int32_t max_high = 0;

    for (int i = 0; i < 3000; i++) {
        float t = (float)i / 1000.0f;

        float amplitude;

        if (t < 1.0f) {
            amplitude = 500.0f;
        }
        else if (t < 2.0f) {
            amplitude = 1000.0f;
        }
        else {
            amplitude = 500.0f;
        }

        int32_t input = (int32_t)(amplitude *
                                  sinf(2.0f * 3.14159265 * 50.0f * t));

        int32_t output = envelope_update(&e, input);

        if (t > 0.5f && t < 1.0f && output > max_low) {
            max_low = output;
        }

        if (t > 1.5f && t < 2.0f && output > max_high) {
            max_high = output;
        }
    }

    /* High amplitude should produce roughly twice the envelope */
    if (max_high < max_low * 1.7f) {
        return 0;
    }

    if (max_high > max_low * 2.3f) {
        return 0;
    }

    return 1;
}


/* Test Response: */

int test_step_response(void){

    struct envelope_t e;

    envelope_init(&e, 1000.0f, 20.0f, 100);

    const int total_samples = 500;
    const int step_sample = 100; 

    const float freq = 50.0f;
    const float amplitude = 1000.0f;
    const float sample_rate = 1000.0f;

    int32_t history[total_samples];

    //0 for the first 100 samples
    // 50 hz afterwards

    for (int i = 0; i < total_samples; i++){
        int32_t input;

        if (i < step_sample){
            input = 0;
        }
        else {
            float t = (float)(i - step_sample) / (sample_rate);

            input = (int32_t)(amplitude * sinf(2.0f * 3.14159265 * freq * t));
        }

        history[i] = envelope_update(&e, input);
    }

    int32_t steady_state = history[total_samples - 1];

    //Define  10% and 90% 
    int32_t lo = (int32_t)(0.1f * steady_state);
    int32_t hi = (int32_t)(0.9f * steady_state);

    //envelope reaches 10%
    int idx_lo = -1;

        for (int i = step_sample; i < total_samples; i++) {

            if (history[i] >= lo) {
                idx_lo = i;
                break;
        }
    }

    //envelope reaches 90%
     int idx_hi = -1;

    for (int i = step_sample; i < total_samples; i++) {

        if (history[i] >= hi) {
            idx_hi = i;
            break;
        }
    }

     if (idx_lo < 0 || idx_hi < 0) {
        printf("FAIL: step response — threshold not reached\n");
        return 0;
    }

    float rise_time_ms = (float)(idx_hi - idx_lo);

    float expected_ms = 80.0f;
    float tolerance_ms = 15.0f;

    int pass = (rise_time_ms >= expected_ms - tolerance_ms) &&
               (rise_time_ms <= expected_ms + tolerance_ms);

    printf("%s: step response — rise time %.1f ms "
           "(expected %.1f ± %.1f ms)\n",
           pass ? "PASS" : "FAIL",
           rise_time_ms,
           expected_ms,
           tolerance_ms);

    return pass;

}


int main(void)
{
    if (test_dc_rejection()) {
        printf("PASS: DC rejection\n");
    }
    else {
        printf("FAIL: DC rejection\n");
    }

    if (test_1hz_attenuation()) {
        printf("PASS: 1 Hz attenuation\n");
    }
    else {
        printf("FAIL: 1 Hz attenuation\n");
    }

    if (test_50hz_response()) {
        printf("PASS: 50 Hz response\n");
    }
    else {
        printf("FAIL: 50 Hz response\n");
    }

    if (test_amplitude_tracking()) {
        printf("PASS: amplitude tracking\n");
    }
    else {
        printf("FAIL: amplitude tracking\n");
    }


    if (test_step_response()) {
    printf("PASS: step response\n");
}
else {
    printf("FAIL: step response\n");
}
    return 0;
}

    
   
