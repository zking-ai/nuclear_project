#ifndef FIRMWARE_STORAGE_H
#define FIRMWARE_STORAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "firmware/config.h"
#include "firmware/metering.h"
#include "firmware/platform.h"

typedef struct
{
    uint32_t version;
    uint32_t publish_period_ms;
    metering_calibration_t calibration;
    protection_thresholds_t thresholds;
    network_credentials_t network;
} persistent_config_t;

typedef struct
{
    persistent_config_t active;
    uint32_t sequence;
} storage_context_t;

void storage_make_defaults(persistent_config_t *config);
bool storage_load(storage_context_t *context);
bool storage_save(storage_context_t *context, const persistent_config_t *config);

#endif
