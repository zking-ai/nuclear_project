#ifndef FIRMWARE_NETWORK_MANAGER_H
#define FIRMWARE_NETWORK_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firmware/config.h"
#include "firmware/metering.h"
#include "firmware/platform.h"

typedef enum
{
    NETWORK_STATE_BOOT = 0,
    NETWORK_STATE_WIFI_CONNECT,
    NETWORK_STATE_MQTT_CONNECT,
    NETWORK_STATE_ONLINE,
    NETWORK_STATE_BACKOFF
} network_state_t;

typedef struct
{
    mqtt_message_t queue[TELEMETRY_QUEUE_DEPTH];
    size_t head;
    size_t tail;
    size_t count;
    network_state_t state;
    network_credentials_t credentials;
    uint32_t next_action_ms;
    uint32_t backoff_ms;
    uint32_t dropped_messages;
} network_manager_t;

void network_manager_init(network_manager_t *manager,
                          const network_credentials_t *credentials);
bool network_manager_queue_snapshot(network_manager_t *manager,
                                    const metering_snapshot_t *snapshot,
                                    uint32_t timestamp_ms);
void network_manager_tick(network_manager_t *manager, uint32_t now_ms);
const char *network_manager_state_name(network_state_t state);

#endif
