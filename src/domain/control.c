#include "control.h"

static ControlCalibration g_calibration = {
    CONTROL_DEFAULT_TEMP_LIMIT,
    CONTROL_DEFAULT_OIL_PRESSURE_LIMIT,
    CONTROL_DEFAULT_HIGH_RPM_WARNING_THRESHOLD,
    CONTROL_DEFAULT_HIGH_TEMP_WARNING_THRESHOLD,
    CONTROL_DEFAULT_TEMP_PERSISTENCE_TICKS,
    CONTROL_DEFAULT_OIL_PERSISTENCE_TICKS,
    CONTROL_DEFAULT_COMBINED_WARNING_PERSISTENCE_TICKS};
static int32_t g_calibration_configured = 0;

static int32_t calibration_valid(const ControlCalibration *calibration)
{
    if (calibration == (const ControlCalibration *)0)
    {
        return 0;
    }

    if ((calibration->temperature_limit <= 0.0f) || (calibration->temperature_limit > 200.0f) ||
        (calibration->oil_pressure_limit < 0.1f) || (calibration->oil_pressure_limit > 10.0f) ||
        (calibration->high_rpm_warning_threshold < 100.0f) || (calibration->high_rpm_warning_threshold > 10000.0f) ||
        (calibration->high_temp_warning_threshold < -50.0f) || (calibration->high_temp_warning_threshold > 200.0f) ||
        (calibration->temp_persistence_ticks == 0U) || (calibration->oil_persistence_ticks == 0U) ||
        (calibration->combined_warning_persistence_ticks == 0U))
    {
        return 0;
    }

    return 1;
}

static void update_fault_counter(int32_t fault_active, uint32_t *counter)
{
    if (counter == (uint32_t *)0)
    {
        return;
    }

    if (fault_active != 0)
    {
        if (*counter < 1000000U)
        {
            *counter += 1U;
        }
    }
    else
    {
        *counter = 0U;
    }
}

static StatusCode transition_engine_mode_by_result(EngineState *engine, int32_t evaluation_result)
{
    if (engine == (EngineState *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if ((engine->mode == ENGINE_STATE_INIT) && (engine->is_running != 0))
    {
        if (engine_transition_mode(engine, ENGINE_STATE_STARTING) != STATUS_OK)
        {
            return STATUS_INTERNAL_ERROR;
        }
    }

    if (engine->mode == ENGINE_STATE_RUNNING)
    {
        if ((evaluation_result == ENGINE_WARNING) || (evaluation_result == ENGINE_SHUTDOWN))
        {
            if (engine_transition_mode(engine, ENGINE_STATE_WARNING) != STATUS_OK)
            {
                return STATUS_INTERNAL_ERROR;
            }
        }
    }
    else if (engine->mode == ENGINE_STATE_WARNING)
    {
        if (evaluation_result == ENGINE_SHUTDOWN)
        {
            if (engine_transition_mode(engine, ENGINE_STATE_SHUTDOWN) != STATUS_OK)
            {
                return STATUS_INTERNAL_ERROR;
            }
        }
    }

    return STATUS_OK;
}

static int32_t mode_to_result_code(const EngineState *engine)
{
    if (engine == (const EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    if (engine->mode == ENGINE_STATE_WARNING)
    {
        return ENGINE_WARNING;
    }
    if (engine->mode == ENGINE_STATE_SHUTDOWN)
    {
        return ENGINE_SHUTDOWN;
    }

    return ENGINE_OK;
}

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

StatusCode evaluate_engine(EngineState *engine, int32_t *evaluation_result)
{
    int32_t shutdown_fault;
    int32_t warning_fault;
    int32_t eval_result_code;

    if ((engine == (EngineState *)0) || (evaluation_result == (int32_t *)0))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    update_fault_counter((engine->temperature > g_calibration.temperature_limit) ? 1 : 0,
                         &engine->fault_counters[ENGINE_FAULT_TEMP]);
    update_fault_counter((engine->oil_pressure < g_calibration.oil_pressure_limit) ? 1 : 0,
                         &engine->fault_counters[ENGINE_FAULT_OIL_PRESSURE]);
    update_fault_counter((engine->rpm >= g_calibration.high_rpm_warning_threshold) &&
                             (engine->temperature >= g_calibration.high_temp_warning_threshold),
                         &engine->fault_counters[ENGINE_FAULT_RPM_TEMP_COMBINED]);

    shutdown_fault = 0;
    if (engine->fault_counters[ENGINE_FAULT_TEMP] >= g_calibration.temp_persistence_ticks)
    {
        shutdown_fault = 1;
    }
    if (engine->fault_counters[ENGINE_FAULT_OIL_PRESSURE] >= g_calibration.oil_persistence_ticks)
    {
        shutdown_fault = 1;
    }

    warning_fault = 0;
    if (engine->fault_counters[ENGINE_FAULT_RPM_TEMP_COMBINED] >= g_calibration.combined_warning_persistence_ticks)
    {
        warning_fault = 1;
    }

    if (shutdown_fault != 0)
    {
        eval_result_code = ENGINE_SHUTDOWN;
    }
    else if (warning_fault != 0)
    {
        eval_result_code = ENGINE_WARNING;
    }
    else
    {
        eval_result_code = ENGINE_OK;
    }

    if (transition_engine_mode_by_result(engine, eval_result_code) != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    *evaluation_result = mode_to_result_code(engine);
    return STATUS_OK;
}

StatusCode compute_control_output(const EngineState *engine, float *control_output)
{
    float output;

    if ((engine == (const EngineState *)0) || (control_output == (float *)0))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    output = 20.0f;
    output += engine->rpm / 100.0f;
    output -= (engine->temperature - 25.0f) * 0.25f;
    output -= (3.0f - engine->oil_pressure) * 10.0f;

    *control_output = clamp_output(output);
    return STATUS_OK;
}

StatusCode control_get_default_calibration(ControlCalibration *calibration_out)
{
    if (calibration_out == (ControlCalibration *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    calibration_out->temperature_limit = CONTROL_DEFAULT_TEMP_LIMIT;
    calibration_out->oil_pressure_limit = CONTROL_DEFAULT_OIL_PRESSURE_LIMIT;
    calibration_out->high_rpm_warning_threshold = CONTROL_DEFAULT_HIGH_RPM_WARNING_THRESHOLD;
    calibration_out->high_temp_warning_threshold = CONTROL_DEFAULT_HIGH_TEMP_WARNING_THRESHOLD;
    calibration_out->temp_persistence_ticks = CONTROL_DEFAULT_TEMP_PERSISTENCE_TICKS;
    calibration_out->oil_persistence_ticks = CONTROL_DEFAULT_OIL_PERSISTENCE_TICKS;
    calibration_out->combined_warning_persistence_ticks = CONTROL_DEFAULT_COMBINED_WARNING_PERSISTENCE_TICKS;
    return STATUS_OK;
}

StatusCode control_get_active_calibration(ControlCalibration *calibration_out)
{
    if (calibration_out == (ControlCalibration *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    *calibration_out = g_calibration;
    return STATUS_OK;
}

StatusCode control_configure_calibration(const ControlCalibration *calibration)
{
    if (calibration_valid(calibration) == 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (g_calibration_configured != 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    g_calibration = *calibration;
    g_calibration_configured = 1;
    return STATUS_OK;
}
