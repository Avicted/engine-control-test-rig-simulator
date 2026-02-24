#include "control.h"

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
