#include "firmware/system.h"

#include <stdio.h>
#include <string.h>

#include "firmware/platform.h"

static void log_snapshot(const metering_snapshot_t *snapshot)
{
    char message[192];
    snprintf(message,
             sizeof(message),
             "vrms=%.2fV irms=%.2fA p=%.2fW e=%.3fWh pf=%.3f",
             snapshot->voltage_rms_v,
             snapshot->current_rms_a,
             snapshot->active_power_w,
             snapshot->accumulated_energy_wh,
             snapshot->power_factor);
    platform_log("metering", message);
}

bool energy_monitor_system_init(energy_monitor_system_t *system)
{
    memset(system, 0, sizeof(*system));

    if (!storage_load(&system->storage))
    {
        storage_make_defaults(&system->config);
        if (!storage_save(&system->storage, &system->config))
        {
            return false;
        }
    }

    system->config = system->storage.active;

    metering_init(&system->metering,
                  &system->config.calibration,
                  &system->config.thresholds);
    network_manager_init(&system->network, &system->config.network);
    ota_manager_init(&system->ota);
    (void)ota_manager_load(&system->ota);

    const uint32_t now_ms = platform_get_tick_ms();
    diag_init(&system->diag, now_ms);
    system->next_sampling_ms = now_ms;
    system->next_network_ms = now_ms;
    system->next_housekeeping_ms = now_ms + 500u;
    return true;
}

void energy_monitor_system_step(energy_monitor_system_t *system, uint32_t now_ms)
{
    if (now_ms >= system->next_sampling_ms)
    {
        adc_frame_t frame;
        if (platform_adc_capture(&frame))
        {
            metering_process_frame(&system->metering, &frame);
            diag_heartbeat(&system->diag, DIAG_TASK_SAMPLING, now_ms);
        }

        if (metering_try_finalize_window(&system->metering, &system->latest_snapshot))
        {
            log_snapshot(&system->latest_snapshot);
            (void)network_manager_queue_snapshot(&system->network,
                                                 &system->latest_snapshot,
                                                 now_ms);
        }

        system->next_sampling_ms = now_ms + 20u;
    }

    if (now_ms >= system->next_network_ms)
    {
        network_manager_tick(&system->network, now_ms);
        diag_heartbeat(&system->diag, DIAG_TASK_NETWORK, now_ms);
        system->next_network_ms = now_ms + 100u;
    }

    if (now_ms >= system->next_housekeeping_ms)
    {
        if (diag_all_tasks_fresh(&system->diag, now_ms, 1500u))
        {
            platform_watchdog_kick();
        }
        diag_heartbeat(&system->diag, DIAG_TASK_HOUSEKEEPING, now_ms);
        system->next_housekeeping_ms = now_ms + 500u;
    }
}
