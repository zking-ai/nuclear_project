#ifndef FIRMWARE_DIAG_H
#define FIRMWARE_DIAG_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    DIAG_TASK_SAMPLING = 0,
    DIAG_TASK_NETWORK,
    DIAG_TASK_HOUSEKEEPING,
    DIAG_TASK_COUNT
} diag_task_id_t;

typedef struct
{
    uint32_t last_heartbeat_ms[DIAG_TASK_COUNT];
    bool watchdog_ok;
} diag_context_t;

void diag_init(diag_context_t *context, uint32_t now_ms);
void diag_heartbeat(diag_context_t *context, diag_task_id_t task, uint32_t now_ms);
bool diag_all_tasks_fresh(const diag_context_t *context, uint32_t now_ms, uint32_t timeout_ms);

#endif
