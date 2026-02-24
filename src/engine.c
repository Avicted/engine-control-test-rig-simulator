#include "engine.h"

int engine_init(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->rpm = 0.0f;
    engine->temperature = 25.0f;
    engine->oil_pressure = 3.0f;
    engine->is_running = 0;

    return ENGINE_OK;
}

int engine_reset(EngineState *engine)
{
    return engine_init(engine);
}

int engine_start(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->is_running = 1;
    return ENGINE_OK;
}

int engine_update(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
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
