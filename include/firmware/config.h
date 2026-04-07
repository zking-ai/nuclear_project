#ifndef FIRMWARE_CONFIG_H
#define FIRMWARE_CONFIG_H

#include <stdint.h>

#define FIRMWARE_VERSION_MAJOR 1u
#define FIRMWARE_VERSION_MINOR 0u

#define ENERGY_SAMPLE_BATCH 64u
#define METERING_WINDOW_MS 1000u
#define TELEMETRY_QUEUE_DEPTH 16u
#define TELEMETRY_TOPIC_MAX 96u
#define TELEMETRY_PAYLOAD_MAX 256u

#define SETTINGS_MAGIC 0x53455453u
#define SETTINGS_VERSION 1u
#define SETTINGS_SLOT_BYTES 512u

#define OTA_MAGIC 0x4F54414Du
#define OTA_METADATA_VERSION 1u
#define OTA_CHUNK_BYTES 256u

#define PLATFORM_FLASH_SETTINGS_BYTES SETTINGS_SLOT_BYTES
#define PLATFORM_FLASH_OTA_META_BYTES 256u
#define PLATFORM_FLASH_APP_SLOT_BYTES 8192u

typedef struct
{
    float over_voltage_v;
    float under_voltage_v;
    float over_current_a;
    float max_frequency_hz;
    float min_frequency_hz;
} protection_thresholds_t;

#endif
