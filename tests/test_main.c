#include "firmware/crc32.h"
#include "firmware/metering.h"
#include "firmware/ota_manager.h"
#include "firmware/platform_mock.h"
#include "firmware/storage.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expect_true(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "test failure: %s\n", message);
        exit(1);
    }
}

static void test_crc32(void)
{
    const char *text = "123456789";
    expect_true(crc32_compute(text, strlen(text)) == 0xCBF43926u, "crc32 mismatch");
}

static void test_storage_roundtrip(void)
{
    platform_mock_reset();

    storage_context_t writer = {0};
    persistent_config_t config;
    storage_make_defaults(&config);
    config.calibration.voltage_gain = 1.02f;
    config.publish_period_ms = 1500u;
    expect_true(storage_save(&writer, &config), "storage save failed");

    storage_context_t reader = {0};
    expect_true(storage_load(&reader), "storage load failed");
    expect_true(reader.active.publish_period_ms == 1500u, "publish period mismatch");
    expect_true(fabsf(reader.active.calibration.voltage_gain - 1.02f) < 0.001f,
                "calibration mismatch");
}

static void test_metering_window(void)
{
    metering_context_t context;
    metering_calibration_t calibration = {1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    protection_thresholds_t thresholds = {242.0f, 198.0f, 12.0f, 51.5f, 48.5f};
    metering_init(&context, &calibration, &thresholds);

    for (unsigned frame_index = 0; frame_index < 50u; ++frame_index)
    {
        adc_frame_t frame;
        frame.count = ENERGY_SAMPLE_BATCH;
        frame.sample_period_us = 313u;
        for (size_t i = 0; i < frame.count; ++i)
        {
            const float t = (float)(frame_index * frame.count + i) * 0.0003125f;
            const float angle = 2.0f * 3.1415926535f * 50.0f * t;
            frame.voltage_samples[i] = 311.0f * sinf(angle);
            frame.current_samples[i] = 11.3f * sinf(angle - 0.12f);
        }
        metering_process_frame(&context, &frame);
    }

    metering_snapshot_t snapshot;
    expect_true(metering_try_finalize_window(&context, &snapshot), "snapshot not ready");
    expect_true(snapshot.valid, "snapshot invalid");
    expect_true(snapshot.voltage_rms_v > 215.0f && snapshot.voltage_rms_v < 225.0f,
                "voltage rms out of range");
    expect_true(snapshot.current_rms_a > 7.5f && snapshot.current_rms_a < 8.5f,
                "current rms out of range");
    expect_true(snapshot.frequency_hz > 48.5f && snapshot.frequency_hz < 51.0f,
                "frequency out of range");
}

static void test_ota_flow(void)
{
    platform_mock_reset();

    ota_manager_t manager;
    ota_manager_init(&manager);
    expect_true(ota_manager_begin(&manager, 2u, 512u), "ota begin failed");

    uint8_t chunk[OTA_CHUNK_BYTES];
    for (size_t i = 0; i < sizeof(chunk); ++i)
    {
        chunk[i] = (uint8_t)i;
    }

    expect_true(ota_manager_write_chunk(&manager, chunk, sizeof(chunk)),
                "ota chunk 1 failed");
    expect_true(ota_manager_write_chunk(&manager, chunk, sizeof(chunk)),
                "ota chunk 2 failed");
    expect_true(ota_manager_finalize(&manager), "ota finalize failed");
    expect_true(manager.status.pending_confirm, "ota should require confirm");
    expect_true(ota_manager_mark_boot_success(&manager), "ota confirm failed");
    expect_true(manager.status.active_slot == OTA_SLOT_B, "ota slot did not switch");
}

int main(void)
{
    test_crc32();
    test_storage_roundtrip();
    test_metering_window();
    test_ota_flow();
    puts("all tests passed");
    return 0;
}

