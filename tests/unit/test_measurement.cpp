#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nexus_measurement.h"
#include "nexus_calibration.h"
#include "nexus_errors.h"

static int test_adc_cal(void) {
    nexus_measurement_init();
    // Apply an affine calibration and verify the conversion
    nexus_measurement_calibrate_adc(0.0f, 1.0f);
    float v = 0.0f;
    nexus_measurement_adc_to_voltage(0, &v);
    if (fabsf(v - 0.0f) > 1e-4f) { printf("  FAIL: adc 0 -> %f\n", v); return 1; }

    nexus_measurement_adc_to_voltage(1023, &v);
    if (v < 1020.0f) { printf("  FAIL: adc 1023 -> %f\n", v); return 1; }
    printf("  PASS: ADC linear conversion\n");

    nexus_measurement_calibrate_adc(4.0f, 1.0021f);
    nexus_measurement_adc_to_voltage(1000, &v);
    float expected = 1000 * 1.0021f + 4.0f;
    if (fabsf(v - expected) > 1e-2f) { printf("  FAIL: cal conversion %f != %f\n", v, expected); return 1; }
    printf("  PASS: ADC offset/gain calibration\n");
    return 0;
}

static int test_frequency(void) {
    nexus_measurement_init();
    // 1 kHz square wave: edges 1 ms apart
    uint32_t edges[] = {0, 1, 2, 3, 4};
    float freq = 0.0f;
    nexus_measurement_compute_frequency(edges, 5, &freq);
    if (fabsf(freq - 1000.0f) > 1.0f) { printf("  FAIL: freq=%f expected 1000\n", freq); return 1; }
    printf("  PASS: frequency computation\n");
    return 0;
}

static int test_duty(void) {
    nexus_measurement_init();
    float duty = 0.0f;
    nexus_measurement_compute_duty_cycle(25, 75, &duty);
    if (fabsf(duty - 25.0f) > 0.01f) { printf("  FAIL: duty=%f expected 25\n", duty); return 1; }
    printf("  PASS: duty cycle computation\n");
    return 0;
}

static int test_rms(void) {
    nexus_measurement_init();
    // RMS of a constant 3.0 is 3.0
    float samples[] = {3.0f, 3.0f, 3.0f, 3.0f};
    float rms = 0.0f;
    nexus_measurement_compute_rms(samples, 4, &rms);
    if (fabsf(rms - 3.0f) > 1e-3f) { printf("  FAIL: rms=%f expected 3\n", rms); return 1; }
    // RMS of a +/-1 square is 1
    float sq[] = {1.0f, -1.0f, 1.0f, -1.0f};
    nexus_measurement_compute_rms(sq, 4, &rms);
    if (fabsf(rms - 1.0f) > 1e-3f) { printf("  FAIL: rms sq=%f expected 1\n", rms); return 1; }
    printf("  PASS: RMS computation\n");
    return 0;
}

static int test_stats(void) {
    nexus_measurement_init();
    float samples[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    nexus_adc_statistics_t st;
    nexus_measurement_compute_statistics(samples, 5, &st);
    if (fabsf(st.min - 1.0f) > 1e-4f) { printf("  FAIL: min\n"); return 1; }
    if (fabsf(st.max - 5.0f) > 1e-4f) { printf("  FAIL: max\n"); return 1; }
    if (fabsf(st.avg - 3.0f) > 1e-4f) { printf("  FAIL: avg\n"); return 1; }
    if (fabsf(st.peak_to_peak - 4.0f) > 1e-4f) { printf("  FAIL: p2p\n"); return 1; }
    printf("  PASS: statistics computation\n");
    return 0;
}

static int test_calibration(void) {
    nexus_calibration_init();
    nexus_calibration_t* cal = nexus_calibration_get();
    if (cal == NULL) { printf("  FAIL: calibration get\n"); return 1; }
    if (nexus_calibration_validate(cal) != NEXUS_OK) { printf("  FAIL: calibration validate default\n"); return 1; }
    printf("  PASS: calibration default valid\n");

    nexus_calibration_set_adc(4.0f, 1.0021f);
    if (nexus_calibration_validate(cal) != NEXUS_ERR_INVALID_CONFIG) {
        printf("  FAIL: mutated calibration should fail checksum\n"); return 1;
    }
    printf("  PASS: calibration checksum detects mutation\n");
    return 0;
}

int test_measurement(void) {
    printf("\n");
    if (test_adc_cal()) return 1;
    if (test_frequency()) return 1;
    if (test_duty()) return 1;
    if (test_rms()) return 1;
    if (test_stats()) return 1;
    if (test_calibration()) return 1;
    return 0;
}
