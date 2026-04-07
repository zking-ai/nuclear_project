#ifndef FIRMWARE_CRC32_H
#define FIRMWARE_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_compute(const void *data, size_t size);
uint32_t crc32_update(uint32_t seed, const void *data, size_t size);

#endif
