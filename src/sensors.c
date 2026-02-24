#include "sensors.h"

int32_t scenario_normal_ramp_up(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->is_running = 1;

    if (engine->rpm < 3200.0f)
    {
        engine->rpm += 120.0f;
        if (engine->rpm > 3200.0f)
        {
            engine->rpm = 3200.0f;
        }
    }

    if (engine->temperature < 88.0f)
    {
        engine->temperature += 0.7f;
        if (engine->temperature > 88.0f)
        {
            engine->temperature = 88.0f;
        }
    }

    engine->oil_pressure = 3.6f;
    return ENGINE_OK;
}

int32_t scenario_overheat(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->is_running = 1;
    engine->rpm = 4200.0f;
    engine->temperature = 103.0f;
    engine->oil_pressure = 3.1f;
    return ENGINE_OK;
}

int32_t scenario_oil_pressure_failure(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->is_running = 1;
    engine->rpm = 2500.0f;
    engine->temperature = 82.0f;
    engine->oil_pressure = 2.1f;
    return ENGINE_OK;
}
