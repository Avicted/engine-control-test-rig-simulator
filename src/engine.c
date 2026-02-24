#include "engine.h"

static const char *const engine_mode_str[] = {"INIT", "STARTING", "RUNNING", "WARNING", "SHUTDOWN"};

static int is_legal_transition(EngineStateMode from_mode, EngineStateMode to_mode)
{
    if (from_mode == to_mode)
    {
        return 1;
    }

    switch (from_mode)
    {
    case ENGINE_STATE_INIT:
        return to_mode == ENGINE_STATE_STARTING;
    case ENGINE_STATE_STARTING:
        return to_mode == ENGINE_STATE_RUNNING;
    case ENGINE_STATE_RUNNING:
        return to_mode == ENGINE_STATE_WARNING;
    case ENGINE_STATE_WARNING:
        return to_mode == ENGINE_STATE_SHUTDOWN;
    case ENGINE_STATE_SHUTDOWN:
        return to_mode == ENGINE_STATE_INIT;
    default:
        return 0; // unknown from_mode
    }
}

int engine_transition_mode(EngineState *engine, EngineStateMode target_mode)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    if (is_legal_transition(engine->mode, target_mode) == 0)
    {
        return ENGINE_ERROR;
    }

    engine->mode = target_mode;
    return ENGINE_OK;
}

int engine_get_mode_string(const EngineState *engine, const char **mode_string)
{
    unsigned int mode_index;

    if ((engine == (const EngineState *)0) || (mode_string == (const char **)0))
    {
        return ENGINE_ERROR;
    }

    mode_index = (unsigned int)engine->mode;
    if (mode_index >= (unsigned int)(sizeof(engine_mode_str) / sizeof(engine_mode_str[0])))
    {
        return ENGINE_ERROR;
    }

    *mode_string = engine_mode_str[mode_index];

    return ENGINE_OK;
}

int engine_init(EngineState *engine)
{
    int index;

    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->rpm = 0.0f;
    engine->temperature = 25.0f;
    engine->oil_pressure = 3.0f;
    engine->is_running = 0;
    engine->mode = ENGINE_STATE_INIT;

    for (index = 0; index < ENGINE_FAULT_COUNTER_COUNT; ++index)
    {
        engine->fault_counters[index] = 0U;
    }

    return ENGINE_OK;
}

int engine_reset(EngineState *engine)
{
    return engine_init(engine);
}

int engine_start(EngineState *engine)
{
    int transition_status;

    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    if (engine->mode != ENGINE_STATE_INIT)
    {
        return ENGINE_ERROR;
    }

    transition_status = engine_transition_mode(engine, ENGINE_STATE_STARTING);
    if (transition_status != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    engine->is_running = 1;
    return ENGINE_OK;
}

int engine_update(EngineState *engine)
{
    int transition_status;

    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    if (engine->mode == ENGINE_STATE_STARTING)
    {
        transition_status = engine_transition_mode(engine, ENGINE_STATE_RUNNING);
        if (transition_status != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (engine->mode == ENGINE_STATE_SHUTDOWN)
    {
        engine->is_running = 0;
        engine->rpm = 0.0f;
        return ENGINE_OK;
    }

    if (!engine->is_running)
    {
        return ENGINE_OK;
    }

    if (engine->rpm < 3000.0f)
    {
        engine->rpm += 150.0f;
        if (engine->rpm > 3000.0f)
        {
            engine->rpm = 3000.0f;
        }
    }

    if (engine->temperature < 90.0f)
    {
        engine->temperature += 0.6f;
        if (engine->temperature > 90.0f)
        {
            engine->temperature = 90.0f;
        }
    }

    if (engine->oil_pressure > 3.4f)
    {
        engine->oil_pressure -= 0.01f;
        if (engine->oil_pressure < 3.4f)
        {
            engine->oil_pressure = 3.4f;
        }
    }

    return ENGINE_OK;
}
