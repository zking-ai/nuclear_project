#include "firmware/platform.h"
#include "firmware/platform_mock.h"
#include "firmware/system.h"

int main(void)
{
    platform_mock_reset();

    energy_monitor_system_t system;
    if (!energy_monitor_system_init(&system))
    {
        platform_log("system", "initialization failed");
        return 1;
    }

    for (unsigned step = 0; step < 600u; ++step)
    {
        const uint32_t now_ms = platform_get_tick_ms();
        energy_monitor_system_step(&system, now_ms);
        platform_mock_advance_time(20u);
    }

    platform_log("system", "host simulation complete");
    return 0;
}
