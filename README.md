# Nuclear Energy Monitor Firmware

This repository contains a firmware-oriented project skeleton for the
"nuclear power station energy monitor" described in the resume.

Implemented areas:

- FreeRTOS-style application decomposition with explicit sampling, network,
  storage and housekeeping tasks.
- Metering pipeline for RMS / power / energy accumulation and anomaly checks.
- Wi-Fi + MQTT reconnect state machine with an offline telemetry ring buffer.
- CRC32-protected dual-copy settings storage.
- A/B OTA image staging metadata and rollback-safe confirmation flow.
- Host-side mock platform so the project can be built and tested without the
  target Bouffalo MCU SDK.

## Build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run host simulation

```powershell
.\build\firmware_host_sim.exe
```

## Layout

- `include/firmware`: public interfaces and shared configuration.
- `src/core`: business logic modules.
- `src/app`: system orchestration and main entry.
- `src/platform/mock`: host mock for ADC, flash, Wi-Fi, MQTT and watchdog.
- `tests`: basic validation for metering, storage and OTA flows.

## Target integration

Replace the implementation in `src/platform/mock/platform_mock.c` with a real
Bouffalo SDK port:

- ADC DMA sampling
- RTC / SysTick source
- Wi-Fi driver and MQTT client
- Flash / OTA partition access
- watchdog kick and reset reason reporting
