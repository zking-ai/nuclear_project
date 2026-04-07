#include "firmware/ota_manager.h"

#include <string.h>

#include "firmware/crc32.h"

typedef struct
{
    uint32_t magic;
    uint32_t metadata_version;
    ota_status_t status;
} ota_metadata_record_t;

static platform_partition_id_t slot_to_partition(ota_slot_t slot)
{
    return (slot == OTA_SLOT_A) ? PLATFORM_PARTITION_APP_A : PLATFORM_PARTITION_APP_B;
}

static bool ota_write_metadata(const ota_manager_t *manager)
{
    ota_metadata_record_t record;
    memset(&record, 0xFF, sizeof(record));
    record.magic = OTA_MAGIC;
    record.metadata_version = OTA_METADATA_VERSION;
    record.status = manager->status;

    if (!platform_flash_erase(PLATFORM_PARTITION_OTA_META))
    {
        return false;
    }
    return platform_flash_write(PLATFORM_PARTITION_OTA_META, 0u, &record, sizeof(record));
}

void ota_manager_init(ota_manager_t *manager)
{
    memset(manager, 0, sizeof(*manager));
    manager->status.version = 1u;
    manager->status.active_slot = OTA_SLOT_A;
    manager->status.pending_slot = OTA_SLOT_A;
}

bool ota_manager_load(ota_manager_t *manager)
{
    ota_metadata_record_t record;
    if (!platform_flash_read(PLATFORM_PARTITION_OTA_META, 0u, &record, sizeof(record)))
    {
        return false;
    }

    if (record.magic != OTA_MAGIC || record.metadata_version != OTA_METADATA_VERSION)
    {
        return false;
    }

    manager->status = record.status;
    return true;
}

bool ota_manager_begin(ota_manager_t *manager, uint32_t version, uint32_t image_size)
{
    const ota_slot_t target_slot =
        (manager->status.active_slot == OTA_SLOT_A) ? OTA_SLOT_B : OTA_SLOT_A;
    const platform_partition_id_t target_partition = slot_to_partition(target_slot);

    if (image_size > platform_flash_partition_size(target_partition))
    {
        return false;
    }
    if (!platform_flash_erase(target_partition))
    {
        return false;
    }

    manager->status.version = version;
    manager->status.pending_slot = target_slot;
    manager->status.pending_confirm = false;
    manager->status.image_size = image_size;
    manager->status.image_crc32 = 0u;
    manager->session_open = true;
    manager->running_crc32 = 0u;
    manager->bytes_written = 0u;
    return true;
}

bool ota_manager_write_chunk(ota_manager_t *manager, const uint8_t *data, size_t size)
{
    if (!manager->session_open)
    {
        return false;
    }
    if ((manager->bytes_written + size) > manager->status.image_size)
    {
        return false;
    }

    const platform_partition_id_t partition = slot_to_partition(manager->status.pending_slot);
    if (!platform_flash_write(partition, manager->bytes_written, data, size))
    {
        return false;
    }

    manager->running_crc32 = crc32_update(manager->running_crc32, data, size);
    manager->bytes_written += (uint32_t)size;
    return true;
}

bool ota_manager_finalize(ota_manager_t *manager)
{
    if (!manager->session_open || manager->bytes_written != manager->status.image_size)
    {
        return false;
    }

    manager->status.image_crc32 = manager->running_crc32;
    manager->status.pending_confirm = true;
    manager->status.boot_attempts = 0u;
    manager->session_open = false;
    return ota_write_metadata(manager);
}

bool ota_manager_mark_boot_success(ota_manager_t *manager)
{
    if (!manager->status.pending_confirm)
    {
        return true;
    }

    manager->status.active_slot = manager->status.pending_slot;
    manager->status.pending_confirm = false;
    manager->status.boot_attempts = 0u;
    return ota_write_metadata(manager);
}
