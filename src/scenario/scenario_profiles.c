#include "control.h"
#include "hal.h"
#include "reporting/logger.h"
#include "scenario/scenario_report.h"
#include "scenario/scenario_profiles.h"
#include "script_parser.h"

static int32_t execute_profile_frames_internal(EngineState *engine,
                                               const uint32_t *tick_values,
                                               const HAL_Frame *sensor_frames,
                                               uint32_t tick_count,
                                               int32_t show_sim,
                                               int32_t show_control,
                                               int32_t show_state,
                                               TickReport *tick_reports,
                                               uint32_t tick_report_capacity,
                                               uint32_t *tick_report_count)
{
    HAL_SensorFrame sensor_frame;
    uint32_t tick_index;
    uint32_t current_tick = 0U;
    StatusCode status;
    int32_t result = ENGINE_OK;

    if ((engine == (EngineState *)0) || (sensor_frames == (const HAL_Frame *)0))
    {
        return ENGINE_ERROR;
    }

    if ((tick_count == 0U) || (tick_count > SCRIPT_PARSER_MAX_TICKS))
    {
        return ENGINE_ERROR;
    }

    if (tick_report_count != (uint32_t *)0)
    {
        *tick_report_count = 0U;
    }

    if ((tick_reports != (TickReport *)0) && (tick_report_capacity < tick_count + 1U))
    {
        return ENGINE_ERROR;
    }

    if (tick_reports != (TickReport *)0)
    {
        tick_reports[0U].tick = 0U;
        tick_reports[0U].rpm = engine->rpm;
        tick_reports[0U].temp = engine->temperature;
        tick_reports[0U].oil = engine->oil_pressure;
        tick_reports[0U].run = engine->is_running;
        tick_reports[0U].result = ENGINE_OK;
        tick_reports[0U].control = 0.0f;
        tick_reports[0U].mode = engine->mode;
    }

    status = engine_start(engine);
    if (status != STATUS_OK)
    {
        return ENGINE_ERROR;
    }

    for (tick_index = 0U; tick_index < tick_count; ++tick_index)
    {
        uint32_t target_tick;
        uint32_t sim_tick;

        target_tick = (tick_values == (const uint32_t *)0) ? (tick_index + 1U) : tick_values[tick_index];
        if (target_tick <= current_tick)
        {
            return ENGINE_ERROR;
        }

        for (sim_tick = current_tick + 1U; sim_tick < target_tick; ++sim_tick)
        {
            status = hal_read_sensors(sim_tick, &sensor_frame);
            if (status == STATUS_TIMEOUT)
            {
                (void)logger_log_tick("HAL", LOG_LEVEL_ERROR, sim_tick, "Timeout detected", 0);
                engine->mode = ENGINE_STATE_SHUTDOWN;
                engine->is_running = 0;
                engine->rpm = 0.0f;
                return ENGINE_SHUTDOWN;
            }
        }

        status = hal_ingest_sensor_frame(&sensor_frames[tick_index], target_tick);
        if (status != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        status = hal_read_sensors(target_tick, &sensor_frame);
        if (status == STATUS_TIMEOUT)
        {
            (void)logger_log_tick("HAL", LOG_LEVEL_ERROR, target_tick, "Timeout detected", 0);
            engine->mode = ENGINE_STATE_SHUTDOWN;
            engine->is_running = 0;
            engine->rpm = 0.0f;
            return ENGINE_SHUTDOWN;
        }
        if (status != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        if (hal_apply_sensors(&sensor_frame, engine) != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        if (evaluate_engine(engine, &result) != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        if (tick_reports != (TickReport *)0)
        {
            float control_output = 0.0f;
            uint32_t report_index = tick_index + 1U;

            if (compute_control_output(engine, &control_output) != STATUS_OK)
            {
                return ENGINE_ERROR;
            }

            tick_reports[report_index].tick = target_tick;
            tick_reports[report_index].rpm = sensor_frame.rpm;
            tick_reports[report_index].temp = sensor_frame.temperature;
            tick_reports[report_index].oil = sensor_frame.oil_pressure;
            tick_reports[report_index].run = sensor_frame.is_running;
            tick_reports[report_index].result = result;
            tick_reports[report_index].control = control_output;
            tick_reports[report_index].mode = engine->mode;
        }

        if ((show_sim != 0) || (show_control != 0) || (show_state != 0))
        {
            if (scenario_report_print_tick_details(target_tick,
                                                   engine,
                                                   result,
                                                   show_sim,
                                                   show_control,
                                                   show_state) != ENGINE_OK)
            {
                return ENGINE_ERROR;
            }
        }

        status = engine_update(engine);
        if (status != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        current_tick = target_tick;
    }

    if (tick_report_count != (uint32_t *)0)
    {
        *tick_report_count = tick_count + 1U;
    }

    return result;
}

int32_t execute_profile_frames(EngineState *engine,
                               const uint32_t *tick_values,
                               const HAL_Frame *sensor_frames,
                               uint32_t tick_count,
                               int32_t show_sim,
                               int32_t show_control,
                               int32_t show_state,
                               TickReport *tick_reports,
                               uint32_t tick_report_capacity,
                               uint32_t *tick_report_count)
{
    return execute_profile_frames_internal(engine,
                                           tick_values,
                                           sensor_frames,
                                           tick_count,
                                           show_sim,
                                           show_control,
                                           show_state,
                                           tick_reports,
                                           tick_report_capacity,
                                           tick_report_count);
}

int32_t execute_profile(EngineState *engine,
                        const uint32_t *tick_values,
                        const float *rpm_values,
                        const float *temp_values,
                        const float *oil_values,
                        const int32_t *run_values,
                        uint32_t tick_count,
                        int32_t show_sim,
                        int32_t show_control,
                        int32_t show_state,
                        TickReport *tick_reports,
                        uint32_t tick_report_capacity,
                        uint32_t *tick_report_count)
{
    HAL_Frame sensor_frames[SCRIPT_PARSER_MAX_TICKS];
    uint32_t tick_index;
    HAL_SensorFrame sensor_frame;

    if ((engine == (EngineState *)0) || (rpm_values == (const float *)0) ||
        (temp_values == (const float *)0) || (oil_values == (const float *)0))
    {
        return ENGINE_ERROR;
    }

    if ((tick_count == 0U) || (tick_count > SCRIPT_PARSER_MAX_TICKS))
    {
        return ENGINE_ERROR;
    }

    for (tick_index = 0U; tick_index < tick_count; ++tick_index)
    {
        int32_t run_flag;

        if (run_values == (const int32_t *)0)
        {
            run_flag = 1;
        }
        else
        {
            run_flag = run_values[tick_index];
        }

        if ((run_flag != 0) && (run_flag != 1))
        {
            return ENGINE_ERROR;
        }

        sensor_frame.rpm = rpm_values[tick_index];
        sensor_frame.temperature = temp_values[tick_index];
        sensor_frame.oil_pressure = oil_values[tick_index];
        sensor_frame.is_running = run_flag;

        if (hal_encode_sensor_frame(&sensor_frame, &sensor_frames[tick_index]) != STATUS_OK)
        {
            return ENGINE_ERROR;
        }
    }

    return execute_profile_frames_internal(engine,
                                           tick_values,
                                           sensor_frames,
                                           tick_count,
                                           show_sim,
                                           show_control,
                                           show_state,
                                           tick_reports,
                                           tick_report_capacity,
                                           tick_report_count);
}

int32_t scenario_normal_operation(EngineState *engine,
                                  int32_t show_sim,
                                  int32_t show_control,
                                  int32_t show_state,
                                  void *tick_reports,
                                  uint32_t tick_report_capacity,
                                  uint32_t *tick_report_count)
{
    static const float rpm_values[] = {2200.0f, 2600.0f, 3000.0f, 3200.0f};
    static const float temp_values[] = {76.0f, 80.0f, 83.0f, 84.5f};
    static const float oil_values[] = {3.2f, 3.1f, 3.0f, 2.9f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_overheat_short_duration(EngineState *engine,
                                         int32_t show_sim,
                                         int32_t show_control,
                                         int32_t show_state,
                                         void *tick_reports,
                                         uint32_t tick_report_capacity,
                                         uint32_t *tick_report_count)
{
    static const float rpm_values[] = {2400.0f, 2400.0f, 2400.0f, 2400.0f};
    static const float temp_values[] = {96.0f, 97.0f, 90.0f, 89.0f};
    static const float oil_values[] = {3.1f, 3.1f, 3.1f, 3.1f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_overheat_persistent(EngineState *engine,
                                     int32_t show_sim,
                                     int32_t show_control,
                                     int32_t show_state,
                                     void *tick_reports,
                                     uint32_t tick_report_capacity,
                                     uint32_t *tick_report_count)
{
    static const float rpm_values[] = {2500.0f, 2500.0f, 2500.0f, 2500.0f};
    static const float temp_values[] = {96.0f, 97.0f, 98.0f, 99.0f};
    static const float oil_values[] = {3.0f, 3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_oil_pressure_persistent(EngineState *engine,
                                         int32_t show_sim,
                                         int32_t show_control,
                                         int32_t show_state,
                                         void *tick_reports,
                                         uint32_t tick_report_capacity,
                                         uint32_t *tick_report_count)
{
    static const float rpm_values[] = {2600.0f, 2600.0f, 2600.0f, 2600.0f};
    static const float temp_values[] = {82.0f, 82.0f, 82.0f, 82.0f};
    static const float oil_values[] = {2.4f, 2.3f, 2.2f, 2.1f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_combined_warning_persistent(EngineState *engine,
                                             int32_t show_sim,
                                             int32_t show_control,
                                             int32_t show_state,
                                             void *tick_reports,
                                             uint32_t tick_report_capacity,
                                             uint32_t *tick_report_count)
{
    static const float rpm_values[] = {3600.0f, 3600.0f, 3300.0f};
    static const float temp_values[] = {86.0f, 87.0f, 80.0f};
    static const float oil_values[] = {3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_cold_start_warmup_and_ramp(EngineState *engine,
                                            int32_t show_sim,
                                            int32_t show_control,
                                            int32_t show_state,
                                            void *tick_reports,
                                            uint32_t tick_report_capacity,
                                            uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        800.0f, 1100.0f, 1400.0f, 1700.0f, 2000.0f,
        2200.0f, 2400.0f, 2600.0f, 2800.0f, 3000.0f,
        3100.0f, 3200.0f, 3300.0f, 3400.0f, 3400.0f,
        3400.0f, 3400.0f, 3300.0f, 3100.0f, 2900.0f,
        2700.0f, 2400.0f, 2100.0f, 1700.0f, 1300.0f};
    static const float temp_values[] = {
        44.0f, 49.0f, 54.0f, 58.5f, 62.0f,
        65.5f, 68.0f, 70.5f, 73.0f, 75.0f,
        77.0f, 78.5f, 80.0f, 81.0f, 82.0f,
        82.5f, 82.0f, 81.0f, 80.0f, 78.5f,
        77.0f, 74.5f, 71.0f, 67.5f, 63.0f};
    static const float oil_values[] = {
        3.8f, 3.7f, 3.7f, 3.6f, 3.5f,
        3.4f, 3.4f, 3.3f, 3.2f, 3.2f,
        3.1f, 3.1f, 3.0f, 3.0f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.1f,
        3.1f, 3.2f, 3.3f, 3.4f, 3.5f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_high_load_warning_then_recovery(EngineState *engine,
                                                 int32_t show_sim,
                                                 int32_t show_control,
                                                 int32_t show_state,
                                                 void *tick_reports,
                                                 uint32_t tick_report_capacity,
                                                 uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        2000.0f, 2400.0f, 2700.0f, 3000.0f, 3200.0f,
        3400.0f, 3500.0f, 3600.0f, 3700.0f, 3500.0f,
        3300.0f, 3100.0f, 2900.0f, 2700.0f, 2500.0f,
        2300.0f, 2100.0f, 2000.0f};
    static const float temp_values[] = {
        70.0f, 73.0f, 75.5f, 78.0f, 80.5f,
        82.5f, 85.0f, 87.0f, 88.0f, 87.0f,
        86.0f, 84.5f, 82.0f, 79.5f, 77.0f,
        74.5f, 72.0f, 70.0f};
    static const float oil_values[] = {
        3.2f, 3.2f, 3.1f, 3.1f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f,
        3.0f, 3.1f, 3.1f, 3.2f, 3.2f,
        3.2f, 3.3f, 3.3f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_oil_pressure_gradual_drain(EngineState *engine,
                                            int32_t show_sim,
                                            int32_t show_control,
                                            int32_t show_state,
                                            void *tick_reports,
                                            uint32_t tick_report_capacity,
                                            uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        2500.0f, 2700.0f, 2900.0f, 3000.0f, 3000.0f,
        3000.0f, 3000.0f, 3000.0f, 2900.0f, 2800.0f,
        2700.0f, 2600.0f, 2500.0f, 2500.0f, 2500.0f,
        2500.0f};
    static const float temp_values[] = {
        74.0f, 76.0f, 77.5f, 78.5f, 79.5f,
        80.0f, 80.0f, 80.5f, 80.0f, 79.5f,
        79.0f, 78.5f, 77.5f, 77.0f, 76.5f,
        76.0f};
    static const float oil_values[] = {
        3.5f, 3.4f, 3.3f, 3.2f, 3.1f,
        3.0f, 2.9f, 2.8f, 2.7f, 2.6f,
        2.5f, 2.4f, 2.3f, 2.2f, 2.1f,
        2.0f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_thermal_runaway_with_load_surge(EngineState *engine,
                                                 int32_t show_sim,
                                                 int32_t show_control,
                                                 int32_t show_state,
                                                 void *tick_reports,
                                                 uint32_t tick_report_capacity,
                                                 uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        2200.0f, 2500.0f, 2800.0f, 3000.0f, 3200.0f,
        3400.0f, 3400.0f, 3400.0f, 3400.0f, 3400.0f,
        3400.0f, 3400.0f, 3400.0f, 3400.0f, 3400.0f};
    static const float temp_values[] = {
        68.0f, 71.0f, 74.0f, 76.5f, 79.0f,
        81.5f, 84.0f, 87.5f, 90.5f, 93.0f,
        95.5f, 97.0f, 98.5f, 99.0f, 99.5f};
    static const float oil_values[] = {
        3.3f, 3.2f, 3.1f, 3.1f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_intermittent_oil_then_combined_fault(EngineState *engine,
                                                      int32_t show_sim,
                                                      int32_t show_control,
                                                      int32_t show_state,
                                                      void *tick_reports,
                                                      uint32_t tick_report_capacity,
                                                      uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        2000.0f, 2200.0f, 2400.0f, 2500.0f, 2500.0f,
        2700.0f, 2900.0f, 3100.0f, 3300.0f, 3500.0f,
        3600.0f, 3500.0f, 3200.0f, 3000.0f, 2800.0f,
        2600.0f};
    static const float temp_values[] = {
        70.0f, 72.0f, 74.0f, 76.0f, 76.0f,
        78.0f, 80.0f, 82.0f, 83.0f, 85.0f,
        86.0f, 87.0f, 85.0f, 83.0f, 80.0f,
        77.0f};
    static const float oil_values[] = {
        3.4f, 3.3f, 2.4f, 2.3f, 2.6f,
        2.8f, 3.0f, 3.0f, 3.0f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f,
        3.1f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}
