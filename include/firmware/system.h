#ifndef FIRMWARE_SYSTEM_H
#define FIRMWARE_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

#include "firmware/diag.h"
#include "firmware/metering.h"
#include "firmware/network_manager.h"
#include "firmware/ota_manager.h"
#include "firmware/storage.h"

typedef struct
{
    storage_context_t storage;
    metering_context_t metering;
    network_manager_t network;
    ota_manager_t ota;
    diag_context_t diag;
    persistent_config_t config;
    metering_snapshot_t latest_snapshot;
    uint32_t next_sampling_ms;
    uint32_t next_network_ms;
    uint32_t next_housekeeping_ms;
} energy_monitor_system_t;

bool energy_monitor_system_init(energy_monitor_system_t *system);
void energy_monitor_system_step(energy_monitor_system_t *system, uint32_t now_ms);

#endif
