#include "firmware/crc32.h"

uint32_t crc32_update(uint32_t seed, const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = ~seed;

    for (size_t i = 0; i < size; ++i)
    {
        crc ^= bytes[i];
        for (uint32_t bit = 0; bit < 8; ++bit)
        {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }

    return ~crc;
}

uint32_t crc32_compute(const void *data, size_t size)
{
    return crc32_update(0u, data, size);
}
