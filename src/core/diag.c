#include "firmware/diag.h"

void diag_init(diag_context_t *context, uint32_t now_ms)
{
    for (int i = 0; i < DIAG_TASK_COUNT; ++i)
    {
        context->last_heartbeat_ms[i] = now_ms;
    }
    context->watchdog_ok = true;
}

void diag_heartbeat(diag_context_t *context, diag_task_id_t task, uint32_t now_ms)
{
    if ((unsigned)task < DIAG_TASK_COUNT)
    {
        context->last_heartbeat_ms[task] = now_ms;
    }
}

bool diag_all_tasks_fresh(const diag_context_t *context, uint32_t now_ms, uint32_t timeout_ms)
{
    for (int i = 0; i < DIAG_TASK_COUNT; ++i)
    {
        if ((now_ms - context->last_heartbeat_ms[i]) > timeout_ms)
        {
            return false;
        }
    }
    return true;
}
