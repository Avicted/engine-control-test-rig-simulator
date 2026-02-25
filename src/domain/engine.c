#include "engine.h"

#include <math.h>    /* isfinite */
#include <stddef.h>  /* NULL */

static const char *const engine_mode_str[] = {"INIT", "STARTING", "RUNNING", "WARNING", "SHUTDOWN"};

/* --- Engine physics configuration (plant model) --- */
static EnginePhysicsConfig g_physics = {
    ENGINE_DEFAULT_TARGET_RPM,
    ENGINE_DEFAULT_TARGET_TEMP,
    ENGINE_DEFAULT_TARGET_OIL,
    ENGINE_DEFAULT_RPM_RAMP_RATE,
    ENGINE_DEFAULT_TEMP_RAMP_RATE,
    ENGINE_DEFAULT_OIL_DECAY_RATE};
static int32_t g_physics_configured = 0;

static int32_t physics_valid(const EnginePhysicsConfig *config)
{
    if (config == NULL)
    {
        return 0;
    }
    if (!isfinite(config->target_rpm) || (config->target_rpm <= 0.0f))
    {
        return 0;
    }
    if (!isfinite(config->target_temperature) || (config->target_temperature <= 0.0f))
    {
        return 0;
    }
    if (!isfinite(config->target_oil_pressure) || (config->target_oil_pressure <= 0.0f))
    {
        return 0;
    }
    if (!isfinite(config->rpm_ramp_rate) || (config->rpm_ramp_rate <= 0.0f))
    {
        return 0;
    }
    if (!isfinite(config->temp_ramp_rate) || (config->temp_ramp_rate <= 0.0f))
    {
        return 0;
    }
    if (!isfinite(config->oil_decay_rate) || (config->oil_decay_rate <= 0.0f))
    {
        return 0;
    }
    return 1;
}

StatusCode engine_get_default_physics(EnginePhysicsConfig *config_out)
{
    if (config_out == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    config_out->target_rpm = ENGINE_DEFAULT_TARGET_RPM;
    config_out->target_temperature = ENGINE_DEFAULT_TARGET_TEMP;
    config_out->target_oil_pressure = ENGINE_DEFAULT_TARGET_OIL;
    config_out->rpm_ramp_rate = ENGINE_DEFAULT_RPM_RAMP_RATE;
    config_out->temp_ramp_rate = ENGINE_DEFAULT_TEMP_RAMP_RATE;
    config_out->oil_decay_rate = ENGINE_DEFAULT_OIL_DECAY_RATE;
    return STATUS_OK;
}

StatusCode engine_get_active_physics(EnginePhysicsConfig *config_out)
{
    if (config_out == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    *config_out = g_physics;
    return STATUS_OK;
}

StatusCode engine_configure_physics(const EnginePhysicsConfig *config)
{
    if (config == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (g_physics_configured != 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (physics_valid(config) == 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    g_physics = *config;
    g_physics_configured = 1;
    return STATUS_OK;
}

StatusCode engine_reset_physics(void)
{
    g_physics.target_rpm = ENGINE_DEFAULT_TARGET_RPM;
    g_physics.target_temperature = ENGINE_DEFAULT_TARGET_TEMP;
    g_physics.target_oil_pressure = ENGINE_DEFAULT_TARGET_OIL;
    g_physics.rpm_ramp_rate = ENGINE_DEFAULT_RPM_RAMP_RATE;
    g_physics.temp_ramp_rate = ENGINE_DEFAULT_TEMP_RAMP_RATE;
    g_physics.oil_decay_rate = ENGINE_DEFAULT_OIL_DECAY_RATE;
    g_physics_configured = 0;
    return STATUS_OK;
}

static int32_t is_legal_transition(EngineStateMode from_mode, EngineStateMode to_mode)
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
        return (to_mode == ENGINE_STATE_RUNNING) || (to_mode == ENGINE_STATE_SHUTDOWN);
    case ENGINE_STATE_RUNNING:
        return (to_mode == ENGINE_STATE_WARNING) || (to_mode == ENGINE_STATE_SHUTDOWN);
    case ENGINE_STATE_WARNING:
        return to_mode == ENGINE_STATE_SHUTDOWN;
    case ENGINE_STATE_SHUTDOWN:
        return to_mode == ENGINE_STATE_INIT;
    default:
        return 0; // unknown from_mode
    }
}

StatusCode engine_transition_mode(EngineState *engine, EngineStateMode target_mode)
{
    if (engine == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (is_legal_transition(engine->mode, target_mode) == 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    engine->mode = target_mode;
    return STATUS_OK;
}

StatusCode engine_get_mode_string(const EngineState *engine, const char **mode_string)
{
    uint32_t mode_index;

    if ((engine == NULL) || (mode_string == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    mode_index = (uint32_t)engine->mode;
    if (mode_index >= (uint32_t)(sizeof(engine_mode_str) / sizeof(engine_mode_str[0])))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    *mode_string = engine_mode_str[mode_index];

    return STATUS_OK;
}

StatusCode engine_init(EngineState *engine)
{
    int32_t index;

    if (engine == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
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

    return STATUS_OK;
}

StatusCode engine_reset(EngineState *engine)
{
    return engine_init(engine);
}

StatusCode engine_start(EngineState *engine)
{
    StatusCode transition_status;

    if (engine == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (engine->mode != ENGINE_STATE_INIT)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    transition_status = engine_transition_mode(engine, ENGINE_STATE_STARTING);
    if (transition_status != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    engine->is_running = 1;
    return STATUS_OK;
}

StatusCode engine_update(EngineState *engine)
{
    if (engine == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (engine->mode == ENGINE_STATE_STARTING)
    {
        StatusCode transition_status;

        transition_status = engine_transition_mode(engine, ENGINE_STATE_RUNNING);
        if (transition_status != STATUS_OK)
        {
            return STATUS_INTERNAL_ERROR;
        }
    }

    if (engine->mode == ENGINE_STATE_SHUTDOWN)
    {
        engine->is_running = 0;
        engine->rpm = 0.0f;
        return STATUS_OK;
    }

    if (!engine->is_running)
    {
        return STATUS_OK;
    }

    if (engine->rpm < g_physics.target_rpm)
    {
        engine->rpm += g_physics.rpm_ramp_rate;
        if (engine->rpm > g_physics.target_rpm)
        {
            engine->rpm = g_physics.target_rpm;
        }
    }

    if (engine->temperature < g_physics.target_temperature)
    {
        engine->temperature += g_physics.temp_ramp_rate;
        if (engine->temperature > g_physics.target_temperature)
        {
            engine->temperature = g_physics.target_temperature;
        }
    }

    if (engine->oil_pressure > g_physics.target_oil_pressure)
    {
        engine->oil_pressure -= g_physics.oil_decay_rate;
        if (engine->oil_pressure < g_physics.target_oil_pressure)
        {
            engine->oil_pressure = g_physics.target_oil_pressure;
        }
    }

    return STATUS_OK;
}
