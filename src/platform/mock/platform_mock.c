#include "firmware/platform.h"
#include "firmware/platform_mock.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    uint8_t data[PLATFORM_FLASH_SETTINGS_BYTES];
} flash_settings_slot_t;

typedef struct
{
    uint8_t data[PLATFORM_FLASH_OTA_META_BYTES];
} flash_ota_slot_t;

typedef struct
{
    uint8_t data[PLATFORM_FLASH_APP_SLOT_BYTES];
} flash_app_slot_t;

typedef struct
{
    uint32_t tick_ms;
    wifi_link_state_t wifi_state;
    mqtt_link_state_t mqtt_state;
    uint32_t wifi_ready_ms;
    uint32_t mqtt_ready_ms;
    bool force_wifi_drop;
    bool force_mqtt_drop;
    flash_settings_slot_t settings_a;
    flash_settings_slot_t settings_b;
    flash_ota_slot_t ota_meta;
    flash_app_slot_t app_a;
    flash_app_slot_t app_b;
} mock_state_t;

static mock_state_t state;

static void *partition_base(platform_partition_id_t partition)
{
    switch (partition)
    {
        case PLATFORM_PARTITION_SETTINGS_A:
            return state.settings_a.data;
        case PLATFORM_PARTITION_SETTINGS_B:
            return state.settings_b.data;
        case PLATFORM_PARTITION_OTA_META:
            return state.ota_meta.data;
        case PLATFORM_PARTITION_APP_A:
            return state.app_a.data;
        case PLATFORM_PARTITION_APP_B:
            return state.app_b.data;
        default:
            return NULL;
    }
}

static size_t partition_capacity(platform_partition_id_t partition)
{
    switch (partition)
    {
        case PLATFORM_PARTITION_SETTINGS_A:
        case PLATFORM_PARTITION_SETTINGS_B:
            return PLATFORM_FLASH_SETTINGS_BYTES;
        case PLATFORM_PARTITION_OTA_META:
            return PLATFORM_FLASH_OTA_META_BYTES;
        case PLATFORM_PARTITION_APP_A:
        case PLATFORM_PARTITION_APP_B:
            return PLATFORM_FLASH_APP_SLOT_BYTES;
        default:
            return 0u;
    }
}

void platform_mock_reset(void)
{
    memset(&state, 0, sizeof(state));
    memset(state.settings_a.data, 0xFF, sizeof(state.settings_a.data));
    memset(state.settings_b.data, 0xFF, sizeof(state.settings_b.data));
    memset(state.ota_meta.data, 0xFF, sizeof(state.ota_meta.data));
    memset(state.app_a.data, 0xFF, sizeof(state.app_a.data));
    memset(state.app_b.data, 0xFF, sizeof(state.app_b.data));
}

void platform_mock_advance_time(uint32_t delta_ms)
{
    state.tick_ms += delta_ms;
}

void platform_mock_force_wifi_drop(bool enable)
{
    state.force_wifi_drop = enable;
}

void platform_mock_force_mqtt_drop(bool enable)
{
    state.force_mqtt_drop = enable;
}

bool platform_adc_capture(adc_frame_t *frame)
{
    const float frequency_hz = 50.0f;
    const float voltage_peak = 311.0f;
    const float current_peak = 11.3f;
    const float phase_rad = 0.12f;
    const float sample_period_s = 0.0003125f;

    frame->count = ENERGY_SAMPLE_BATCH;
    frame->sample_period_us = 313u;

    for (size_t i = 0; i < frame->count; ++i)
    {
        const float t = ((float)state.tick_ms / 1000.0f) + (sample_period_s * (float)i);
        const float angle = 2.0f * 3.1415926535f * frequency_hz * t;
        frame->voltage_samples[i] = voltage_peak * sinf(angle);
        frame->current_samples[i] = current_peak * sinf(angle - phase_rad);
    }

    return true;
}

uint32_t platform_get_tick_ms(void)
{
    return state.tick_ms;
}

void platform_watchdog_kick(void)
{
    platform_log("watchdog", "kick");
}

void platform_network_poll(uint32_t now_ms)
{
    if (state.force_wifi_drop)
    {
        state.wifi_state = WIFI_LINK_DOWN;
        state.mqtt_state = MQTT_LINK_DOWN;
        return;
    }

    if (state.wifi_state == WIFI_LINK_CONNECTING && now_ms >= state.wifi_ready_ms)
    {
        state.wifi_state = WIFI_LINK_READY;
    }

    if (state.force_mqtt_drop)
    {
        state.mqtt_state = MQTT_LINK_DOWN;
        return;
    }

    if (state.mqtt_state == MQTT_LINK_CONNECTING && now_ms >= state.mqtt_ready_ms)
    {
        state.mqtt_state = MQTT_LINK_READY;
    }
}

void platform_log(const char *module, const char *message)
{
    printf("[%08lu] %-10s %s\n",
           (unsigned long)state.tick_ms,
           module,
           message);
}

wifi_link_state_t platform_wifi_get_state(void)
{
    return state.wifi_state;
}

bool platform_wifi_request_connect(const network_credentials_t *credentials)
{
    (void)credentials;
    if (state.force_wifi_drop)
    {
        return false;
    }
    if (state.wifi_state == WIFI_LINK_READY || state.wifi_state == WIFI_LINK_CONNECTING)
    {
        return true;
    }

    state.wifi_state = WIFI_LINK_CONNECTING;
    state.wifi_ready_ms = state.tick_ms + 300u;
    return true;
}

void platform_wifi_disconnect(void)
{
    state.wifi_state = WIFI_LINK_DOWN;
}

mqtt_link_state_t platform_mqtt_get_state(void)
{
    return state.mqtt_state;
}

bool platform_mqtt_request_connect(const network_credentials_t *credentials)
{
    (void)credentials;
    if (state.force_mqtt_drop || state.wifi_state != WIFI_LINK_READY)
    {
        return false;
    }
    if (state.mqtt_state == MQTT_LINK_READY || state.mqtt_state == MQTT_LINK_CONNECTING)
    {
        return true;
    }

    state.mqtt_state = MQTT_LINK_CONNECTING;
    state.mqtt_ready_ms = state.tick_ms + 200u;
    return true;
}

void platform_mqtt_disconnect(void)
{
    state.mqtt_state = MQTT_LINK_DOWN;
}

bool platform_mqtt_publish(const mqtt_message_t *message)
{
    if (state.force_mqtt_drop || state.mqtt_state != MQTT_LINK_READY)
    {
        return false;
    }

    char buffer[384];
    snprintf(buffer, sizeof(buffer), "%s => %s", message->topic, message->payload);
    platform_log("mqtt", buffer);
    return true;
}

size_t platform_flash_partition_size(platform_partition_id_t partition)
{
    return partition_capacity(partition);
}

bool platform_flash_read(platform_partition_id_t partition,
                         size_t offset,
                         void *buffer,
                         size_t size)
{
    uint8_t *base = (uint8_t *)partition_base(partition);
    const size_t capacity = partition_capacity(partition);
    if (base == NULL || (offset + size) > capacity)
    {
        return false;
    }

    memcpy(buffer, base + offset, size);
    return true;
}

bool platform_flash_write(platform_partition_id_t partition,
                          size_t offset,
                          const void *buffer,
                          size_t size)
{
    uint8_t *base = (uint8_t *)partition_base(partition);
    const size_t capacity = partition_capacity(partition);
    if (base == NULL || (offset + size) > capacity)
    {
        return false;
    }

    memcpy(base + offset, buffer, size);
    return true;
}

bool platform_flash_erase(platform_partition_id_t partition)
{
    void *base = partition_base(partition);
    const size_t capacity = partition_capacity(partition);
    if (base == NULL)
    {
        return false;
    }

    memset(base, 0xFF, capacity);
    return true;
}
