#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

#include "status.h"

#define ENGINE_OK 0
#define ENGINE_WARNING 1
#define ENGINE_SHUTDOWN 2
#define ENGINE_ERROR -1

typedef enum
{
    ENGINE_STATE_INIT,
    ENGINE_STATE_STARTING,
    ENGINE_STATE_RUNNING,
    ENGINE_STATE_WARNING,
    ENGINE_STATE_SHUTDOWN
} EngineStateMode;

typedef enum
{
    ENGINE_FAULT_TEMP = 0,
    ENGINE_FAULT_OIL_PRESSURE = 1,
    ENGINE_FAULT_RPM_TEMP_COMBINED = 2,
    ENGINE_FAULT_COUNTER_COUNT = 3
} EngineFaultCounter;

typedef struct
{
    float rpm;
    float temperature;
    float oil_pressure;
    int32_t is_running;
    EngineStateMode mode;
    uint32_t fault_counters[ENGINE_FAULT_COUNTER_COUNT];
} EngineState;

_Static_assert(sizeof(int32_t) == 4U, "int32_t must be 32-bit");
_Static_assert(sizeof(uint32_t) == 4U, "uint32_t must be 32-bit");
_Static_assert(ENGINE_STATE_INIT == 0, "EngineStateMode ordinal contract changed");
_Static_assert(ENGINE_STATE_SHUTDOWN == 4, "EngineStateMode ordinal contract changed");
_Static_assert(ENGINE_FAULT_COUNTER_COUNT == 3, "Unexpected fault counter count");
_Static_assert((sizeof(((EngineState *)0)->fault_counters) / sizeof(uint32_t)) == ENGINE_FAULT_COUNTER_COUNT,
               "EngineState fault counter array size mismatch");

StatusCode engine_init(EngineState *engine);

/*
 * PRE: engine != NULL.
 * POST: engine is reinitialized to deterministic baseline values.
 */
StatusCode engine_reset(EngineState *engine);

/*
 * PRE: engine != NULL and engine->mode == ENGINE_STATE_INIT.
 * POST: engine transitions to STARTING and run flag is enabled.
 */
StatusCode engine_start(EngineState *engine);

/*
 * PRE: engine != NULL.
 * POST: engine performs one deterministic update step and legal transitions.
 */
StatusCode engine_update(EngineState *engine);

/*
 * PRE: engine != NULL and requested transition is legal.
 * POST: engine->mode == target_mode when STATUS_OK is returned.
 */
StatusCode engine_transition_mode(EngineState *engine, EngineStateMode target_mode);

/*
 * PRE: engine != NULL and mode_string != NULL.
 * POST: *mode_string points to stable mode text for the current engine mode.
 */
StatusCode engine_get_mode_string(const EngineState *engine, const char **mode_string);

#endif
