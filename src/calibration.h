#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>

typedef struct {
    int32_t baseline;
    int32_t mcv;
} calibration;

int32_t calculate_average(const int32_t *samples, uint16_t count);

int calibration_calculate(calibration *cal,
                          const int32_t *relaxed_samples,
                          uint16_t relaxed_count,
                          const int32_t *contracted_samples,
                          uint16_t contracted_count);

#endif