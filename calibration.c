#include "calibration.h"

int32_t calculate_average(const int32_t *samples, uint16_t count)
{
    if (count == 0) {
        return 0;
    }

    int32_t sum = 0;

    for (int i = 0; i < count; i++) {
        sum = sum + samples[i];
    }

    return sum / count;
}

int calibration_calculate(calibration *cal,
                          const int32_t *relaxed_samples,
                          uint16_t relaxed_count,
                          const int32_t *contracted_samples,
                          uint16_t contracted_count)
{
    if (relaxed_count == 0 || contracted_count == 0) {
        return 1;
    }

    cal->baseline = calculate_average(relaxed_samples, relaxed_count);
    cal->mcv = calculate_average(contracted_samples, contracted_count);

    if (cal->mcv <= cal->baseline) {
        return 1;
    }

    return 0;
}