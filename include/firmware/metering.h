#ifndef FIRMWARE_METERING_H
#define FIRMWARE_METERING_H

#include <stdbool.h>
#include <stdint.h>

#include "firmware/config.h"
#include "firmware/platform.h"

typedef struct
{
    float voltage_gain;
    float current_gain;
    float voltage_offset;
    float current_offset;
    float phase_compensation;
} metering_calibration_t;

typedef struct
{
    bool valid;
    bool over_voltage;
    bool under_voltage;
    bool over_current;
    bool frequency_fault;
    float voltage_rms_v;
    float current_rms_a;
    float active_power_w;
    float apparent_power_va;
    float reactive_power_var;
    float power_factor;
    float frequency_hz;
    double accumulated_energy_wh;
    uint32_t sample_count;
} metering_snapshot_t;

typedef struct
{
    metering_calibration_t calibration;
    protection_thresholds_t thresholds;
    double sum_v_sq;
    double sum_i_sq;
    double sum_p;
    double accumulated_energy_wh;
    uint32_t sample_count;
    uint32_t upward_zero_crossings;
    uint32_t window_elapsed_ms;
    float prev_voltage;
} metering_context_t;

void metering_init(metering_context_t *context,
                   const metering_calibration_t *calibration,
                   const protection_thresholds_t *thresholds);
void metering_process_frame(metering_context_t *context, const adc_frame_t *frame);
bool metering_try_finalize_window(metering_context_t *context, metering_snapshot_t *snapshot);

#endif
