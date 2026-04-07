cmake_minimum_required(VERSION 3.20)

project(nuclear_energy_monitor_firmware C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(FIRMWARE_BUILD_HOST_SIM "Build host simulator and tests" ON)

add_library(firmware_core
    src/app/system.c
    src/core/crc32.c
    src/core/diag.c
    src/core/metering.c
    src/core/network_manager.c
    src/core/ota_manager.c
    src/core/storage.c
    src/platform/mock/platform_mock.c
)

target_include_directories(firmware_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(firmware_core PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(firmware_core PUBLIC m)
endif()

if(FIRMWARE_BUILD_HOST_SIM)
    add_executable(firmware_host_sim src/app/main.c)
    target_link_libraries(firmware_host_sim PRIVATE firmware_core)

    enable_testing()
    add_executable(firmware_tests tests/test_main.c)
    target_link_libraries(firmware_tests PRIVATE firmware_core)
    add_test(NAME firmware_tests COMMAND firmware_tests)
endif()
