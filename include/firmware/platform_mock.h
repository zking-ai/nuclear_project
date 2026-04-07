#ifndef FIRMWARE_PLATFORM_MOCK_H
#define FIRMWARE_PLATFORM_MOCK_H

#include <stdbool.h>
#include <stdint.h>

void platform_mock_reset(void);
void platform_mock_advance_time(uint32_t delta_ms);
void platform_mock_force_wifi_drop(bool enable);
void platform_mock_force_mqtt_drop(bool enable);

#endif
