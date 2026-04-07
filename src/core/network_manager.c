#include "firmware/network_manager.h"

#include <stdio.h>
#include <string.h>

static void set_backoff(network_manager_t *manager, uint32_t now_ms)
{
    if (manager->backoff_ms == 0u)
    {
        manager->backoff_ms = 1000u;
    }
    else if (manager->backoff_ms < 8000u)
    {
        manager->backoff_ms *= 2u;
    }

    manager->state = NETWORK_STATE_BACKOFF;
    manager->next_action_ms = now_ms + manager->backoff_ms;
}

static bool queue_push(network_manager_t *manager, const mqtt_message_t *message)
{
    if (manager->count == TELEMETRY_QUEUE_DEPTH)
    {
        manager->head = (manager->head + 1u) % TELEMETRY_QUEUE_DEPTH;
        manager->count--;
        manager->dropped_messages++;
    }

    manager->queue[manager->tail] = *message;
    manager->tail = (manager->tail + 1u) % TELEMETRY_QUEUE_DEPTH;
    manager->count++;
    return true;
}

static bool queue_pop(network_manager_t *manager, mqtt_message_t *message)
{
    if (manager->count == 0u)
    {
        return false;
    }

    *message = manager->queue[manager->head];
    manager->head = (manager->head + 1u) % TELEMETRY_QUEUE_DEPTH;
    manager->count--;
    return true;
}

void network_manager_init(network_manager_t *manager,
                          const network_credentials_t *credentials)
{
    memset(manager, 0, sizeof(*manager));
    manager->credentials = *credentials;
    manager->state = NETWORK_STATE_BOOT;
}

bool network_manager_queue_snapshot(network_manager_t *manager,
                                    const metering_snapshot_t *snapshot,
                                    uint32_t timestamp_ms)
{
    mqtt_message_t message;
    memset(&message, 0, sizeof(message));

    snprintf(message.topic,
             sizeof(message.topic),
             "%s/telemetry",
             manager->credentials.topic_prefix);

    snprintf(message.payload,
             sizeof(message.payload),
             "{\"ts\":%lu,\"vrms\":%.2f,\"irms\":%.2f,\"p\":%.2f,"
             "\"e_wh\":%.3f,\"pf\":%.3f,\"alarm\":%u}",
             (unsigned long)timestamp_ms,
             snapshot->voltage_rms_v,
             snapshot->current_rms_a,
             snapshot->active_power_w,
             snapshot->accumulated_energy_wh,
             snapshot->power_factor,
             (unsigned)(snapshot->over_voltage || snapshot->under_voltage ||
                        snapshot->over_current || snapshot->frequency_fault));

    message.qos = 1u;
    message.retained = false;
    return queue_push(manager, &message);
}

void network_manager_tick(network_manager_t *manager, uint32_t now_ms)
{
    platform_network_poll(now_ms);

    switch (manager->state)
    {
        case NETWORK_STATE_BOOT:
            manager->state = NETWORK_STATE_WIFI_CONNECT;
            manager->backoff_ms = 0u;
            break;

        case NETWORK_STATE_WIFI_CONNECT:
            if (platform_wifi_get_state() == WIFI_LINK_READY)
            {
                manager->state = NETWORK_STATE_MQTT_CONNECT;
                break;
            }
            if (platform_wifi_get_state() == WIFI_LINK_DOWN &&
                !platform_wifi_request_connect(&manager->credentials))
            {
                set_backoff(manager, now_ms);
            }
            break;

        case NETWORK_STATE_MQTT_CONNECT:
            if (platform_wifi_get_state() != WIFI_LINK_READY)
            {
                set_backoff(manager, now_ms);
                break;
            }
            if (platform_mqtt_get_state() == MQTT_LINK_READY)
            {
                manager->state = NETWORK_STATE_ONLINE;
                manager->backoff_ms = 0u;
                break;
            }
            if (platform_mqtt_get_state() == MQTT_LINK_DOWN &&
                !platform_mqtt_request_connect(&manager->credentials))
            {
                set_backoff(manager, now_ms);
            }
            break;

        case NETWORK_STATE_ONLINE:
            if (platform_wifi_get_state() != WIFI_LINK_READY ||
                platform_mqtt_get_state() != MQTT_LINK_READY)
            {
                platform_mqtt_disconnect();
                set_backoff(manager, now_ms);
                break;
            }

            while (manager->count > 0u)
            {
                mqtt_message_t message;
                if (!queue_pop(manager, &message))
                {
                    break;
                }
                if (!platform_mqtt_publish(&message))
                {
                    queue_push(manager, &message);
                    set_backoff(manager, now_ms);
                    break;
                }
            }
            break;

        case NETWORK_STATE_BACKOFF:
            if (now_ms >= manager->next_action_ms)
            {
                platform_wifi_disconnect();
                platform_mqtt_disconnect();
                manager->state = NETWORK_STATE_WIFI_CONNECT;
            }
            break;
    }
}

const char *network_manager_state_name(network_state_t state)
{
    switch (state)
    {
        case NETWORK_STATE_BOOT:
            return "BOOT";
        case NETWORK_STATE_WIFI_CONNECT:
            return "WIFI_CONNECT";
        case NETWORK_STATE_MQTT_CONNECT:
            return "MQTT_CONNECT";
        case NETWORK_STATE_ONLINE:
            return "ONLINE";
        case NETWORK_STATE_BACKOFF:
            return "BACKOFF";
        default:
            return "UNKNOWN";
    }
}
