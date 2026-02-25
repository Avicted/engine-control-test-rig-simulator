#include "control.h"

#include <math.h>   /* isfinite */
#include <stddef.h> /* NULL */

/*
 * NOTE: All module-level state assumes single-threaded execution.
 * If threaded scenarios are introduced (see MISRA Dir 5.1/5.2),
 * add volatile or _Atomic qualifiers and appropriate synchronisation.
 */
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
    if (calibration == NULL)
    {
        return 0;
    }

    if (!isfinite(calibration->temperature_limit) || !isfinite(calibration->oil_pressure_limit) ||
        !isfinite(calibration->high_rpm_warning_threshold) || !isfinite(calibration->high_temp_warning_threshold))
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
    if (counter == NULL)
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
    if (engine == NULL)
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
    else
    {
        /* No action required for other modes - MISRA 15.7 terminal else */
    }

    return STATUS_OK;
}

static int32_t mode_to_result_code(const EngineState *engine)
{
    if (engine == NULL)
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

    if ((engine == NULL) || (evaluation_result == NULL))
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

    if ((engine == NULL) || (control_output == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    output = CONTROL_OUTPUT_BASE_BIAS;
    output += engine->rpm / CONTROL_OUTPUT_RPM_DIVISOR;
    output -= (engine->temperature - CONTROL_OUTPUT_TEMP_REFERENCE) * CONTROL_OUTPUT_TEMP_COEFFICIENT;
    output -= (CONTROL_OUTPUT_OIL_REFERENCE - engine->oil_pressure) * CONTROL_OUTPUT_OIL_COEFFICIENT;

    *control_output = clamp_output(output);
    return STATUS_OK;
}

StatusCode control_get_default_calibration(ControlCalibration *calibration_out)
{
    if (calibration_out == NULL)
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
    if (calibration_out == NULL)
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

StatusCode control_reset_calibration(void)
{
    g_calibration.temperature_limit = CONTROL_DEFAULT_TEMP_LIMIT;
    g_calibration.oil_pressure_limit = CONTROL_DEFAULT_OIL_PRESSURE_LIMIT;
    g_calibration.high_rpm_warning_threshold = CONTROL_DEFAULT_HIGH_RPM_WARNING_THRESHOLD;
    g_calibration.high_temp_warning_threshold = CONTROL_DEFAULT_HIGH_TEMP_WARNING_THRESHOLD;
    g_calibration.temp_persistence_ticks = CONTROL_DEFAULT_TEMP_PERSISTENCE_TICKS;
    g_calibration.oil_persistence_ticks = CONTROL_DEFAULT_OIL_PERSISTENCE_TICKS;
    g_calibration.combined_warning_persistence_ticks = CONTROL_DEFAULT_COMBINED_WARNING_PERSISTENCE_TICKS;
    g_calibration_configured = 0;
    return STATUS_OK;
}
