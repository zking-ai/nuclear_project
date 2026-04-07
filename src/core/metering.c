#include "firmware/metering.h"

#include <math.h>
#include <string.h>

static float clamp_pf(float value)
{
    if (value > 1.0f)
    {
        return 1.0f;
    }
    if (value < -1.0f)
    {
        return -1.0f;
    }
    return value;
}

void metering_init(metering_context_t *context,
                   const metering_calibration_t *calibration,
                   const protection_thresholds_t *thresholds)
{
    memset(context, 0, sizeof(*context));
    context->calibration = *calibration;
    context->thresholds = *thresholds;
}

void metering_process_frame(metering_context_t *context, const adc_frame_t *frame)
{
    for (size_t i = 0; i < frame->count; ++i)
    {
        const float v = (frame->voltage_samples[i] - context->calibration.voltage_offset) *
                        context->calibration.voltage_gain;
        const float i_raw = (frame->current_samples[i] - context->calibration.current_offset) *
                            context->calibration.current_gain;
        const float current = i_raw + (context->calibration.phase_compensation * context->prev_voltage * 0.001f);

        context->sum_v_sq += (double)v * (double)v;
        context->sum_i_sq += (double)current * (double)current;
        context->sum_p += (double)v * (double)current;

        if (context->prev_voltage < 0.0f && v >= 0.0f)
        {
            context->upward_zero_crossings++;
        }

        context->prev_voltage = v;
    }

    context->sample_count += (uint32_t)frame->count;
    context->window_elapsed_ms +=
        (uint32_t)(((uint64_t)frame->sample_period_us * frame->count) / 1000u);
}

bool metering_try_finalize_window(metering_context_t *context, metering_snapshot_t *snapshot)
{
    if (context->sample_count == 0u || context->window_elapsed_ms < METERING_WINDOW_MS)
    {
        return false;
    }

    memset(snapshot, 0, sizeof(*snapshot));

    const float sample_count = (float)context->sample_count;
    const float voltage_rms = sqrtf((float)(context->sum_v_sq / sample_count));
    const float current_rms = sqrtf((float)(context->sum_i_sq / sample_count));
    const float active_power = (float)(context->sum_p / sample_count);
    const float apparent_power = voltage_rms * current_rms;
    const float power_factor = (apparent_power > 0.001f) ? clamp_pf(active_power / apparent_power) : 0.0f;
    const float reactive_sq = fmaxf(0.0f, apparent_power * apparent_power - active_power * active_power);
    const float reactive_power = sqrtf(reactive_sq);
    const float hours = (float)context->window_elapsed_ms / 3600000.0f;

    context->accumulated_energy_wh += (double)active_power * (double)hours;

    snapshot->valid = true;
    snapshot->voltage_rms_v = voltage_rms;
    snapshot->current_rms_a = current_rms;
    snapshot->active_power_w = active_power;
    snapshot->apparent_power_va = apparent_power;
    snapshot->reactive_power_var = reactive_power;
    snapshot->power_factor = power_factor;
    snapshot->frequency_hz =
        (context->window_elapsed_ms > 0u) ? (context->upward_zero_crossings * 1000.0f) /
                                                (float)context->window_elapsed_ms
                                          : 0.0f;
    snapshot->accumulated_energy_wh = context->accumulated_energy_wh;
    snapshot->sample_count = context->sample_count;

    snapshot->over_voltage = voltage_rms > context->thresholds.over_voltage_v;
    snapshot->under_voltage = voltage_rms < context->thresholds.under_voltage_v;
    snapshot->over_current = current_rms > context->thresholds.over_current_a;
    snapshot->frequency_fault =
        (snapshot->frequency_hz > context->thresholds.max_frequency_hz) ||
        (snapshot->frequency_hz < context->thresholds.min_frequency_hz);

    context->sum_v_sq = 0.0;
    context->sum_i_sq = 0.0;
    context->sum_p = 0.0;
    context->sample_count = 0u;
    context->upward_zero_crossings = 0u;
    context->window_elapsed_ms = 0u;

    return true;
}
