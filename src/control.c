#include "control.h"

static float clamp_output(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 100.0f)
    {
        return 100.0f;
    }
    return value;
}

int evaluate_engine(const EngineState *engine)
{
    if (engine == (const EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    if (engine->temperature > MAX_TEMP)
    {
        return ENGINE_SHUTDOWN;
    }

    if (engine->oil_pressure < MIN_OIL_PRESSURE)
    {
        return ENGINE_SHUTDOWN;
    }

    if (engine->rpm >= HIGH_RPM_WARNING_THRESHOLD && engine->temperature >= HIGH_TEMP_WARNING_THRESHOLD)
    {
        return ENGINE_WARNING;
    }

    return ENGINE_OK;
}

int compute_control_output(const EngineState *engine, float *control_output)
{
    float output;

    if ((engine == (const EngineState *)0) || (control_output == (float *)0))
    {
        return ENGINE_ERROR;
    }

    output = 20.0f;
    output += engine->rpm / 100.0f;
    output -= (engine->temperature - 25.0f) * 0.25f;
    output -= (3.0f - engine->oil_pressure) * 10.0f;

    *control_output = clamp_output(output);
    return ENGINE_OK;
}
