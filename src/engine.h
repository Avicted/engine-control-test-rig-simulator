#ifndef ENGINE_H
#define ENGINE_H

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
    int is_running;
    EngineStateMode mode;
    unsigned int fault_counters[ENGINE_FAULT_COUNTER_COUNT];
} EngineState;

int engine_init(EngineState *engine);
int engine_reset(EngineState *engine);
int engine_start(EngineState *engine);
int engine_update(EngineState *engine);
int engine_transition_mode(EngineState *engine, EngineStateMode target_mode);
int engine_get_mode_string(const EngineState *engine, const char **mode_string);

#endif
