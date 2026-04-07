#include "firmware/storage.h"

#include <string.h>

#include "firmware/crc32.h"

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t sequence;
    uint32_t payload_size;
    uint32_t crc32;
    persistent_config_t payload;
} storage_record_t;

static bool read_record(platform_partition_id_t partition, storage_record_t *record)
{
    memset(record, 0, sizeof(*record));
    if (!platform_flash_read(partition, 0u, record, sizeof(*record)))
    {
        return false;
    }

    if (record->magic != SETTINGS_MAGIC || record->version != SETTINGS_VERSION)
    {
        return false;
    }
    if (record->payload_size != sizeof(record->payload))
    {
        return false;
    }

    return crc32_compute(&record->payload, sizeof(record->payload)) == record->crc32;
}

void storage_make_defaults(persistent_config_t *config)
{
    memset(config, 0, sizeof(*config));
    config->version = SETTINGS_VERSION;
    config->publish_period_ms = METERING_WINDOW_MS;

    config->calibration.voltage_gain = 1.0f;
    config->calibration.current_gain = 1.0f;
    config->calibration.voltage_offset = 0.0f;
    config->calibration.current_offset = 0.0f;
    config->calibration.phase_compensation = 0.0f;

    config->thresholds.over_voltage_v = 242.0f;
    config->thresholds.under_voltage_v = 198.0f;
    config->thresholds.over_current_a = 12.0f;
    config->thresholds.max_frequency_hz = 51.5f;
    config->thresholds.min_frequency_hz = 48.5f;

    strcpy(config->network.ssid, "plant-ap");
    strcpy(config->network.password, "nuclear-monitor");
    strcpy(config->network.broker_host, "10.10.20.15");
    config->network.broker_port = 1883u;
    strcpy(config->network.client_id, "energy-monitor-01");
    strcpy(config->network.topic_prefix, "plant/energy/monitor01");
}

bool storage_load(storage_context_t *context)
{
    storage_record_t a;
    storage_record_t b;
    const bool valid_a = read_record(PLATFORM_PARTITION_SETTINGS_A, &a);
    const bool valid_b = read_record(PLATFORM_PARTITION_SETTINGS_B, &b);

    if (!valid_a && !valid_b)
    {
        return false;
    }

    const storage_record_t *selected = NULL;
    if (valid_a && valid_b)
    {
        selected = (a.sequence >= b.sequence) ? &a : &b;
    }
    else
    {
        selected = valid_a ? &a : &b;
    }

    context->active = selected->payload;
    context->sequence = selected->sequence;
    return true;
}

bool storage_save(storage_context_t *context, const persistent_config_t *config)
{
    storage_record_t current_a;
    storage_record_t current_b;
    const bool valid_a = read_record(PLATFORM_PARTITION_SETTINGS_A, &current_a);
    const bool valid_b = read_record(PLATFORM_PARTITION_SETTINGS_B, &current_b);

    platform_partition_id_t target = PLATFORM_PARTITION_SETTINGS_A;
    uint32_t next_sequence = 1u;

    if (valid_a && (!valid_b || current_a.sequence <= current_b.sequence))
    {
        target = PLATFORM_PARTITION_SETTINGS_B;
        next_sequence = current_a.sequence + 1u;
    }
    else if (valid_b)
    {
        target = PLATFORM_PARTITION_SETTINGS_A;
        next_sequence = current_b.sequence + 1u;
    }

    storage_record_t record;
    memset(&record, 0xFF, sizeof(record));
    record.magic = SETTINGS_MAGIC;
    record.version = SETTINGS_VERSION;
    record.sequence = next_sequence;
    record.payload_size = sizeof(record.payload);
    record.payload = *config;
    record.crc32 = crc32_compute(&record.payload, sizeof(record.payload));

    if (!platform_flash_erase(target))
    {
        return false;
    }
    if (!platform_flash_write(target, 0u, &record, sizeof(record)))
    {
        return false;
    }

    context->active = *config;
    context->sequence = next_sequence;
    return true;
}
