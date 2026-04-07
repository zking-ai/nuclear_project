#ifndef FIRMWARE_OTA_MANAGER_H
#define FIRMWARE_OTA_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firmware/config.h"
#include "firmware/platform.h"

typedef enum
{
    OTA_SLOT_A = 0,
    OTA_SLOT_B
} ota_slot_t;

typedef struct
{
    uint32_t version;
    ota_slot_t active_slot;
    ota_slot_t pending_slot;
    bool pending_confirm;
    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t boot_attempts;
} ota_status_t;

typedef struct
{
    ota_status_t status;
    bool session_open;
    uint32_t running_crc32;
    uint32_t bytes_written;
} ota_manager_t;

void ota_manager_init(ota_manager_t *manager);
bool ota_manager_load(ota_manager_t *manager);
bool ota_manager_begin(ota_manager_t *manager, uint32_t version, uint32_t image_size);
bool ota_manager_write_chunk(ota_manager_t *manager, const uint8_t *data, size_t size);
bool ota_manager_finalize(ota_manager_t *manager);
bool ota_manager_mark_boot_success(ota_manager_t *manager);

#endif
