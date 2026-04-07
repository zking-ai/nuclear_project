#ifndef FIRMWARE_PLATFORM_H
#define FIRMWARE_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firmware/config.h"

typedef struct
{
    float voltage_samples[ENERGY_SAMPLE_BATCH];
    float current_samples[ENERGY_SAMPLE_BATCH];
    uint32_t sample_period_us;
    size_t count;
} adc_frame_t;

typedef struct
{
    char ssid[32];
    char password[64];
    char broker_host[64];
    uint16_t broker_port;
    char client_id[32];
    char topic_prefix[48];
} network_credentials_t;

typedef struct
{
    char topic[TELEMETRY_TOPIC_MAX];
    char payload[TELEMETRY_PAYLOAD_MAX];
    uint8_t qos;
    bool retained;
} mqtt_message_t;

typedef enum
{
    WIFI_LINK_DOWN = 0,
    WIFI_LINK_CONNECTING,
    WIFI_LINK_READY
} wifi_link_state_t;

typedef enum
{
    MQTT_LINK_DOWN = 0,
    MQTT_LINK_CONNECTING,
    MQTT_LINK_READY
} mqtt_link_state_t;

typedef enum
{
    PLATFORM_PARTITION_SETTINGS_A = 0,
    PLATFORM_PARTITION_SETTINGS_B,
    PLATFORM_PARTITION_OTA_META,
    PLATFORM_PARTITION_APP_A,
    PLATFORM_PARTITION_APP_B,
    PLATFORM_PARTITION_COUNT
} platform_partition_id_t;

bool platform_adc_capture(adc_frame_t *frame);
uint32_t platform_get_tick_ms(void);
void platform_watchdog_kick(void);
void platform_network_poll(uint32_t now_ms);
void platform_log(const char *module, const char *message);

wifi_link_state_t platform_wifi_get_state(void);
bool platform_wifi_request_connect(const network_credentials_t *credentials);
void platform_wifi_disconnect(void);

mqtt_link_state_t platform_mqtt_get_state(void);
bool platform_mqtt_request_connect(const network_credentials_t *credentials);
void platform_mqtt_disconnect(void);
bool platform_mqtt_publish(const mqtt_message_t *message);

size_t platform_flash_partition_size(platform_partition_id_t partition);
bool platform_flash_read(platform_partition_id_t partition,
                         size_t offset,
                         void *buffer,
                         size_t size);
bool platform_flash_write(platform_partition_id_t partition,
                          size_t offset,
                          const void *buffer,
                          size_t size);
bool platform_flash_erase(platform_partition_id_t partition);

#endif
