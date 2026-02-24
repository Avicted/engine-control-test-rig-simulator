#include <stdio.h>
#include <string.h>

#include "control.h"
#include "engine.h"
#include "hal.h"
#include "logger.h"
#include "requirements.h"
#include "script_parser.h"
#include "test_runner.h"
#include "version.h"

#define TEST_LINE_BUFFER_SIZE 256
#define RESULT_COLUMN_WIDTH 8
#define MAX_PROFILE_TICKS 26U

#define ANSI_RESET "\x1b[0m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"

typedef struct
{
    unsigned int tick;
    float rpm;
    float temp;
    float oil;
    int run;
    int result;
    float control;
    EngineStateMode mode;
} TickReport;

static int print_line(const char *line)
{
    int write_result;

    if (line == (const char *)0)
    {
        return ENGINE_ERROR;
    }

    write_result = fputs(line, stdout);
    if (write_result < 0)
    {
        return ENGINE_ERROR;
    }

    return ENGINE_OK;
}

static int print_test_separator(void)
{
    return print_line("------------------------------------------------------------\n");
}

static const char *result_to_string(int result)
{
    if (result == ENGINE_OK)
    {
        return "OK";
    }
    if (result == ENGINE_WARNING)
    {
        return "WARNING";
    }
    if (result == ENGINE_SHUTDOWN)
    {
        return "SHUTDOWN";
    }
    return "ERROR";
}

static const char *mode_to_string(EngineStateMode mode)
{
    if (mode == ENGINE_STATE_INIT)
    {
        return "INIT";
    }
    if (mode == ENGINE_STATE_STARTING)
    {
        return "STARTING";
    }
    if (mode == ENGINE_STATE_RUNNING)
    {
        return "RUNNING";
    }
    if (mode == ENGINE_STATE_WARNING)
    {
        return "WARNING";
    }
    if (mode == ENGINE_STATE_SHUTDOWN)
    {
        return "SHUTDOWN";
    }

    return "UNKNOWN";
}

static const char *result_to_display_string(int result, int use_color)
{
    if (use_color == 0)
    {
        return result_to_string(result);
    }

    if (result == ENGINE_OK)
    {
        return ANSI_GREEN "OK" ANSI_RESET;
    }
    if (result == ENGINE_WARNING)
    {
        return ANSI_YELLOW "WARNING" ANSI_RESET;
    }
    if (result == ENGINE_SHUTDOWN)
    {
        return ANSI_RED "SHUTDOWN" ANSI_RESET;
    }

    return ANSI_RED "ERROR" ANSI_RESET;
}

static const char *pass_fail_display(int is_pass, int use_color)
{
    if (is_pass != 0)
    {
        if (use_color != 0)
        {
            return ANSI_GREEN "PASS" ANSI_RESET;
        }
        return "PASS";
    }

    if (use_color != 0)
    {
        return ANSI_RED "FAIL" ANSI_RESET;
    }
    return "FAIL";
}

static int print_tick_details(unsigned int tick,
                              const EngineState *engine,
                              int result,
                              int show_sim,
                              int show_control,
                              int show_state)
{
    char line[TEST_LINE_BUFFER_SIZE];
    HAL_ControlFrame control_frame;
    int written;

    if (engine == (const EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line,
                       sizeof(line),
                       "TICK | %u  result=%s\n",
                       tick,
                       result_to_string(result));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    if (show_sim != 0)
    {
        written = snprintf(line,
                           sizeof(line),
                           "SIM  | rpm=%7.2f  temp=%6.2f  oil=%5.2f  run=%d\n",
                           engine->rpm,
                           engine->temperature,
                           engine->oil_pressure,
                           engine->is_running);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (show_control != 0)
    {
        int control_status;
        float control_output = 0.0f;

        control_status = compute_control_output(engine, &control_output);
        if (control_status != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        control_frame.control_output = control_output;
        control_frame.emit_control_line = 1;
        if (hal_write_actuators(&control_frame) != STATUS_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (show_state != 0)
    {
        const char *mode_string;

        if (engine_get_mode_string(engine, &mode_string) != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        written = snprintf(line,
                           sizeof(line),
                           "STATE| mode=%-8s  temp_cnt=%u  oil_cnt=%u  combo_cnt=%u\n",
                           mode_string,
                           engine->fault_counters[ENGINE_FAULT_TEMP],
                           engine->fault_counters[ENGINE_FAULT_OIL_PRESSURE],
                           engine->fault_counters[ENGINE_FAULT_RPM_TEMP_COMBINED]);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    return ENGINE_OK;
}

static int execute_profile(EngineState *engine,
                           const uint32_t *tick_values,
                           const float *rpm_values,
                           const float *temp_values,
                           const float *oil_values,
                           const int32_t *run_values,
                           uint32_t tick_count,
                           int show_sim,
                           int show_control,
                           int show_state,
                           TickReport *tick_reports,
                           uint32_t tick_report_capacity,
                           uint32_t *tick_report_count)
{
    HAL_SensorFrame sensor_frame;
    uint32_t tick_index;
    StatusCode status;
    int result = ENGINE_OK;

    if ((engine == (EngineState *)0) || (rpm_values == (const float *)0) ||
        (temp_values == (const float *)0) || (oil_values == (const float *)0))
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

    /* Record tick 0: INIT state before engine_start */
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
        int run_flag;

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

        sensor_frame.is_running = run_flag;
        sensor_frame.rpm = rpm_values[tick_index];
        sensor_frame.temperature = temp_values[tick_index];
        sensor_frame.oil_pressure = oil_values[tick_index];

        if (hal_read_sensors(&sensor_frame) != STATUS_OK)
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
            uint32_t report_index = tick_index + 1U; /* +1: index 0 is the pre-start INIT tick */

            if (compute_control_output(engine, &control_output) != STATUS_OK)
            {
                return ENGINE_ERROR;
            }

            tick_reports[report_index].tick =
                (tick_values == (const uint32_t *)0) ? (tick_index + 1U) : tick_values[tick_index];
            tick_reports[report_index].rpm = rpm_values[tick_index];
            tick_reports[report_index].temp = temp_values[tick_index];
            tick_reports[report_index].oil = oil_values[tick_index];
            tick_reports[report_index].run = run_flag;
            tick_reports[report_index].result = result;
            tick_reports[report_index].control = control_output;
            tick_reports[report_index].mode = engine->mode;
        }

        if ((show_sim != 0) || (show_control != 0) || (show_state != 0))
        {
            uint32_t tick_value;

            tick_value = (tick_values == (const uint32_t *)0) ? (tick_index + 1U) : tick_values[tick_index];
            if (print_tick_details(tick_value, engine, result, show_sim, show_control, show_state) != ENGINE_OK)
            {
                return ENGINE_ERROR;
            }
        }

        status = engine_update(engine);
        if (status != STATUS_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (tick_report_count != (uint32_t *)0)
    {
        *tick_report_count = tick_count + 1U; /* +1 for the pre-start INIT tick at index 0 */
    }

    return result;
}

static int scenario_normal_operation(EngineState *engine,
                                     int show_sim,
                                     int show_control,
                                     int show_state,
                                     void *tick_reports,
                                     unsigned int tick_report_capacity,
                                     unsigned int *tick_report_count)
{
    static const float rpm_values[] = {2200.0f, 2600.0f, 3000.0f, 3200.0f};
    static const float temp_values[] = {76.0f, 80.0f, 83.0f, 84.5f};
    static const float oil_values[] = {3.2f, 3.1f, 3.0f, 2.9f};

    return execute_profile(engine,
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_overheat_short_duration(EngineState *engine,
                                            int show_sim,
                                            int show_control,
                                            int show_state,
                                            void *tick_reports,
                                            unsigned int tick_report_capacity,
                                            unsigned int *tick_report_count)
{
    static const float rpm_values[] = {2400.0f, 2400.0f, 2400.0f, 2400.0f};
    static const float temp_values[] = {96.0f, 97.0f, 90.0f, 89.0f};
    static const float oil_values[] = {3.1f, 3.1f, 3.1f, 3.1f};

    return execute_profile(engine,
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_overheat_persistent(EngineState *engine,
                                        int show_sim,
                                        int show_control,
                                        int show_state,
                                        void *tick_reports,
                                        unsigned int tick_report_capacity,
                                        unsigned int *tick_report_count)
{
    static const float rpm_values[] = {2500.0f, 2500.0f, 2500.0f, 2500.0f};
    static const float temp_values[] = {96.0f, 97.0f, 98.0f, 99.0f};
    static const float oil_values[] = {3.0f, 3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_oil_pressure_persistent(EngineState *engine,
                                            int show_sim,
                                            int show_control,
                                            int show_state,
                                            void *tick_reports,
                                            unsigned int tick_report_capacity,
                                            unsigned int *tick_report_count)
{
    static const float rpm_values[] = {2600.0f, 2600.0f, 2600.0f, 2600.0f};
    static const float temp_values[] = {82.0f, 82.0f, 82.0f, 82.0f};
    static const float oil_values[] = {2.4f, 2.3f, 2.2f, 2.1f};

    return execute_profile(engine,
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_combined_warning_persistent(EngineState *engine,
                                                int show_sim,
                                                int show_control,
                                                int show_state,
                                                void *tick_reports,
                                                unsigned int tick_report_capacity,
                                                unsigned int *tick_report_count)
{
    static const float rpm_values[] = {3600.0f, 3600.0f, 3300.0f};
    static const float temp_values[] = {86.0f, 87.0f, 80.0f};
    static const float oil_values[] = {3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_cold_start_warmup_and_ramp(EngineState *engine,
                                               int show_sim,
                                               int show_control,
                                               int show_state,
                                               void *tick_reports,
                                               unsigned int tick_report_capacity,
                                               unsigned int *tick_report_count)
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
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_high_load_warning_then_recovery(EngineState *engine,
                                                    int show_sim,
                                                    int show_control,
                                                    int show_state,
                                                    void *tick_reports,
                                                    unsigned int tick_report_capacity,
                                                    unsigned int *tick_report_count)
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
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_oil_pressure_gradual_drain(EngineState *engine,
                                               int show_sim,
                                               int show_control,
                                               int show_state,
                                               void *tick_reports,
                                               unsigned int tick_report_capacity,
                                               unsigned int *tick_report_count)
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
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_thermal_runaway_with_load_surge(EngineState *engine,
                                                    int show_sim,
                                                    int show_control,
                                                    int show_state,
                                                    void *tick_reports,
                                                    unsigned int tick_report_capacity,
                                                    unsigned int *tick_report_count)
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
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static int scenario_intermittent_oil_then_combined_fault(EngineState *engine,
                                                         int show_sim,
                                                         int show_control,
                                                         int show_state,
                                                         void *tick_reports,
                                                         unsigned int tick_report_capacity,
                                                         unsigned int *tick_report_count)
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
                           (const unsigned int *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int *)0,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

static StatusCode print_json_scenario_object(const char *scenario_name,
                                             const char *requirement_id,
                                             const TickReport *tick_reports,
                                             unsigned int tick_count,
                                             int expected_result,
                                             int actual_result,
                                             int has_expected)
{
    char line[TEST_LINE_BUFFER_SIZE];
    int written;
    unsigned int index;
    int expected_value;
    int is_pass;

    if ((scenario_name == (const char *)0) || (requirement_id == (const char *)0) ||
        (tick_reports == (const TickReport *)0))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    expected_value = has_expected != 0 ? expected_result : actual_result;
    is_pass = (expected_value == actual_result) ? 1 : 0;

    if (print_line("    {\n") != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"scenario\": \"%s\",\n", scenario_name);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"requirement_id\": \"%s\",\n", requirement_id);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    if (print_line("      \"ticks\": [\n") != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    for (index = 0U; index < tick_count; ++index)
    {
        written = snprintf(line,
                           sizeof(line),
                           "        {\"tick\": %u, \"rpm\": %.2f, \"temp\": %.2f, \"oil\": %.2f, \"run\": %d, "
                           "\"result\": \"%s\", \"control\": %.2f, \"engine_mode\": \"%s\"}%s\n",
                           tick_reports[index].tick,
                           tick_reports[index].rpm,
                           tick_reports[index].temp,
                           tick_reports[index].oil,
                           tick_reports[index].run,
                           result_to_string(tick_reports[index].result),
                           tick_reports[index].control,
                           mode_to_string(tick_reports[index].mode),
                           (index + 1U < tick_count) ? "," : "");
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return STATUS_BUFFER_OVERFLOW;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return STATUS_IO_ERROR;
        }
    }

    if (print_line("      ],\n") != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"expected\": \"%s\",\n", result_to_string(expected_value));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"actual\": \"%s\",\n", result_to_string(actual_result));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"pass\": %s\n", (is_pass != 0) ? "true" : "false");
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    return (print_line("    }") == ENGINE_OK) ? STATUS_OK : STATUS_IO_ERROR;
}

static StatusCode print_json_report_header(void)
{
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (print_line("{\n") != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "  \"schema_version\": \"%s\",\n", SIM_SCHEMA_VERSION);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "  \"software_version\": \"%s\",\n", SIM_SOFTWARE_VERSION);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    return (print_line("  \"scenarios\": [\n") == ENGINE_OK) ? STATUS_OK : STATUS_IO_ERROR;
}

static StatusCode print_json_report_footer(int passed, int total)
{
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (print_line("\n  ],\n") != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line,
                       sizeof(line),
                       "  \"summary\": {\"passed\": %d, \"total\": %d}\n}\n",
                       passed,
                       total);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    return (print_line(line) == ENGINE_OK) ? STATUS_OK : STATUS_IO_ERROR;
}

static int run_test_case(const TestCase *test_case,
                         int show_sim,
                         int use_color,
                         int show_control,
                         int show_state,
                         int json_output,
                         int json_prefix_comma)
{
    EngineState engine;
    int actual_result;
    StatusCode status;
    TickReport tick_reports[MAX_PROFILE_TICKS];
    unsigned int tick_report_count = 0U;
    int local_show_sim;
    int local_show_control;
    int local_show_state;

    if ((test_case == (const TestCase *)0) || (test_case->test_name == (const char *)0) ||
        (test_case->requirement_id == (const char *)0) ||
        (test_case->scenario_func == (int32_t (*)(EngineState *, int32_t, int32_t, int32_t, void *, uint32_t, uint32_t *))0))
    {
        return 0;
    }

    status = engine_reset(&engine);
    if (status != STATUS_OK)
    {
        return 0;
    }

    if (json_output == 0)
    {
        if (print_test_separator() != ENGINE_OK)
        {
            return 0;
        }
    }

    local_show_sim = (json_output != 0) ? 0 : show_sim;
    local_show_control = (json_output != 0) ? 0 : show_control;
    local_show_state = (json_output != 0) ? 0 : show_state;

    actual_result = test_case->scenario_func(&engine,
                                             local_show_sim,
                                             local_show_control,
                                             local_show_state,
                                             tick_reports,
                                             MAX_PROFILE_TICKS,
                                             &tick_report_count);
    if (actual_result == ENGINE_ERROR)
    {
        return 0;
    }

    if (json_output != 0)
    {
        if (json_prefix_comma != 0)
        {
            if (print_line(",\n") != ENGINE_OK)
            {
                return 0;
            }
        }

        if (print_json_scenario_object(test_case->test_name,
                                       test_case->requirement_id,
                                       tick_reports,
                                       tick_report_count,
                                       test_case->expected_result,
                                       actual_result,
                                       1) != STATUS_OK)
        {
            return 0;
        }
    }
    else
    {
        char line[TEST_LINE_BUFFER_SIZE];
        int written;

        written = snprintf(line,
                           sizeof(line),
                           "%s | %s | expected=%s | actual=%s | %s\n",
                           test_case->requirement_id,
                           test_case->test_name,
                           result_to_display_string(test_case->expected_result, use_color),
                           result_to_display_string(actual_result, use_color),
                           pass_fail_display(actual_result == test_case->expected_result, use_color));
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return 0;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return 0;
        }
    }

    return (actual_result == test_case->expected_result) ? 1 : 0;
}

static int valid_scenario_name(const char *name)
{
    size_t name_len;

    if (name == (const char *)0)
    {
        return 0;
    }

    name_len = strlen(name);
    if ((name_len == 0U) || (name_len >= (size_t)MAX_SCENARIO_NAME_LEN))
    {
        return 0;
    }

    return 1;
}

static int validate_hal_negative_cases(void)
{
    if (hal_read_sensors((HAL_SensorFrame *)0) != STATUS_INVALID_ARGUMENT)
    {
        return ENGINE_ERROR;
    }

    return ENGINE_OK;
}

static const char *requirement_for_named_scenario(const char *name)
{
    if (name == (const char *)0)
    {
        return REQ_ENG_003;
    }

    if (strncmp(name, "normal", MAX_SCENARIO_NAME_LEN) == 0)
    {
        return REQ_ENG_003;
    }
    if (strncmp(name, "overheat", MAX_SCENARIO_NAME_LEN) == 0)
    {
        return REQ_ENG_001;
    }
    if (strncmp(name, "pressure_failure", MAX_SCENARIO_NAME_LEN) == 0)
    {
        return REQ_ENG_002;
    }
    if (strncmp(name, "cold_start", MAX_SCENARIO_NAME_LEN) == 0)
    {
        return REQ_ENG_003;
    }
    if (strncmp(name, "high_load", MAX_SCENARIO_NAME_LEN) == 0)
    {
        return REQ_ENG_003;
    }
    if (strncmp(name, "oil_drain", MAX_SCENARIO_NAME_LEN) == 0)
    {
        return REQ_ENG_002;
    }
    if (strncmp(name, "thermal_runaway", MAX_SCENARIO_NAME_LEN) == 0)
    {
        return REQ_ENG_001;
    }
    if (strncmp(name, "intermittent_oil", MAX_SCENARIO_NAME_LEN) == 0)
    {
        return REQ_ENG_002;
    }

    return REQ_ENG_003;
}

StatusCode run_all_tests_with_json(int32_t show_sim,
                                   int32_t use_color,
                                   int32_t show_control,
                                   int32_t show_state,
                                   int32_t json_output)
{
    const TestCase tests[MAX_TESTS] = {
        {"normal_operation", REQ_ENG_003, scenario_normal_operation, ENGINE_OK},
        {"overheat_lt_persistence", REQ_ENG_001, scenario_overheat_short_duration, ENGINE_OK},
        {"overheat_ge_persistence", REQ_ENG_001, scenario_overheat_persistent, ENGINE_SHUTDOWN},
        {"oil_low_ge_persistence", REQ_ENG_002, scenario_oil_pressure_persistent, ENGINE_SHUTDOWN},
        {"combined_warning_persistence", REQ_ENG_003, scenario_combined_warning_persistent, ENGINE_WARNING},
        {"cold_start", REQ_ENG_003, scenario_cold_start_warmup_and_ramp, ENGINE_OK},
        {"high_load", REQ_ENG_003, scenario_high_load_warning_then_recovery, ENGINE_WARNING},
        {"oil_drain", REQ_ENG_002, scenario_oil_pressure_gradual_drain, ENGINE_SHUTDOWN},
        {"thermal_runaway", REQ_ENG_001, scenario_thermal_runaway_with_load_surge, ENGINE_SHUTDOWN},
        {"intermittent_oil", REQ_ENG_002, scenario_intermittent_oil_then_combined_fault, ENGINE_WARNING}};
    int passed = 0;
    const int total = MAX_TESTS;
    int index;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (hal_init() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    if (validate_hal_negative_cases() != ENGINE_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (json_output == 0)
    {
        if (log_event_with_options("INFO", "Running automated validation tests", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        if (print_json_report_header() != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }

    for (index = 0; index < MAX_TESTS; ++index)
    {
        passed += run_test_case(&tests[index],
                                show_sim,
                                use_color,
                                show_control,
                                show_state,
                                json_output,
                                (json_output != 0) && (index > 0));
    }

    if (json_output != 0)
    {
        if (print_json_report_footer(passed, total) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        if (print_test_separator() != ENGINE_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }

        written = snprintf(line, sizeof(line), "Summary: %d/%d tests passed\n", passed, total);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }

        if (passed == total)
        {
            if (log_event_with_options("INFO", "All tests passed", use_color) != STATUS_OK)
            {
                (void)hal_shutdown();
                return STATUS_INTERNAL_ERROR;
            }
            if (hal_shutdown() != STATUS_OK)
            {
                return STATUS_INTERNAL_ERROR;
            }
            return STATUS_OK;
        }

        if (log_event_with_options("ERROR", "One or more tests failed", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }

    if (hal_shutdown() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    return (passed == total) ? STATUS_OK : STATUS_INTERNAL_ERROR;
}

StatusCode run_all_tests_with_full_options(int32_t show_sim,
                                           int32_t use_color,
                                           int32_t show_control,
                                           int32_t show_state)
{
    return run_all_tests_with_json(show_sim, use_color, show_control, show_state, 0);
}

StatusCode run_all_tests(void)
{
    return run_all_tests_with_full_options(0, 0, 0, 0);
}

StatusCode run_all_tests_with_output(int32_t show_sim)
{
    return run_all_tests_with_full_options(show_sim, 0, 0, 0);
}

StatusCode run_all_tests_with_options(int32_t show_sim, int32_t use_color)
{
    return run_all_tests_with_full_options(show_sim, use_color, 0, 0);
}

StatusCode run_named_scenario_with_json(const char *name,
                                        int32_t show_sim,
                                        int32_t use_color,
                                        int32_t show_control,
                                        int32_t show_state,
                                        int32_t json_output)
{
    EngineState engine;
    int result;
    StatusCode status;
    StatusCode log_status;
    int expected_result;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;
    TickReport tick_reports[MAX_PROFILE_TICKS];
    unsigned int tick_report_count = 0U;
    int local_show_sim;
    int local_show_control;
    int local_show_state;

    if (hal_init() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    if (valid_scenario_name(name) == 0)
    {
        (void)hal_shutdown();
        return STATUS_INVALID_ARGUMENT;
    }

    status = engine_reset(&engine);
    if (status != STATUS_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    local_show_sim = (json_output != 0) ? 0 : show_sim;
    local_show_control = (json_output != 0) ? 0 : show_control;
    local_show_state = (json_output != 0) ? 0 : show_state;

    if (strncmp(name, "normal", MAX_SCENARIO_NAME_LEN) == 0)
    {
        expected_result = ENGINE_OK;
        result = scenario_normal_operation(&engine,
                                           local_show_sim,
                                           local_show_control,
                                           local_show_state,
                                           tick_reports,
                                           MAX_PROFILE_TICKS,
                                           &tick_report_count);
    }
    else if (strncmp(name, "overheat", MAX_SCENARIO_NAME_LEN) == 0)
    {
        expected_result = ENGINE_SHUTDOWN;
        result = scenario_overheat_persistent(&engine,
                                              local_show_sim,
                                              local_show_control,
                                              local_show_state,
                                              tick_reports,
                                              MAX_PROFILE_TICKS,
                                              &tick_report_count);
    }
    else if (strncmp(name, "pressure_failure", MAX_SCENARIO_NAME_LEN) == 0)
    {
        expected_result = ENGINE_SHUTDOWN;
        result = scenario_oil_pressure_persistent(&engine,
                                                  local_show_sim,
                                                  local_show_control,
                                                  local_show_state,
                                                  tick_reports,
                                                  MAX_PROFILE_TICKS,
                                                  &tick_report_count);
    }
    else if (strncmp(name, "cold_start", MAX_SCENARIO_NAME_LEN) == 0)
    {
        expected_result = ENGINE_OK;
        result = scenario_cold_start_warmup_and_ramp(&engine,
                                                     local_show_sim,
                                                     local_show_control,
                                                     local_show_state,
                                                     tick_reports,
                                                     MAX_PROFILE_TICKS,
                                                     &tick_report_count);
    }
    else if (strncmp(name, "high_load", MAX_SCENARIO_NAME_LEN) == 0)
    {
        expected_result = ENGINE_WARNING;
        result = scenario_high_load_warning_then_recovery(&engine,
                                                          local_show_sim,
                                                          local_show_control,
                                                          local_show_state,
                                                          tick_reports,
                                                          MAX_PROFILE_TICKS,
                                                          &tick_report_count);
    }
    else if (strncmp(name, "oil_drain", MAX_SCENARIO_NAME_LEN) == 0)
    {
        expected_result = ENGINE_SHUTDOWN;
        result = scenario_oil_pressure_gradual_drain(&engine,
                                                     local_show_sim,
                                                     local_show_control,
                                                     local_show_state,
                                                     tick_reports,
                                                     MAX_PROFILE_TICKS,
                                                     &tick_report_count);
    }
    else if (strncmp(name, "thermal_runaway", MAX_SCENARIO_NAME_LEN) == 0)
    {
        expected_result = ENGINE_SHUTDOWN;
        result = scenario_thermal_runaway_with_load_surge(&engine,
                                                          local_show_sim,
                                                          local_show_control,
                                                          local_show_state,
                                                          tick_reports,
                                                          MAX_PROFILE_TICKS,
                                                          &tick_report_count);
    }
    else if (strncmp(name, "intermittent_oil", MAX_SCENARIO_NAME_LEN) == 0)
    {
        expected_result = ENGINE_WARNING;
        result = scenario_intermittent_oil_then_combined_fault(&engine,
                                                               local_show_sim,
                                                               local_show_control,
                                                               local_show_state,
                                                               tick_reports,
                                                               MAX_PROFILE_TICKS,
                                                               &tick_report_count);
    }
    else
    {
        (void)hal_shutdown();
        return STATUS_INVALID_ARGUMENT;
    }

    if (result == ENGINE_ERROR)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (json_output != 0)
    {
        if (print_json_report_header() != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (print_json_scenario_object(name,
                                       requirement_for_named_scenario(name),
                                       tick_reports,
                                       tick_report_count,
                                       expected_result,
                                       result,
                                       1) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (print_json_report_footer((result == expected_result) ? 1 : 0, 1) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (hal_shutdown() != STATUS_OK)
        {
            return STATUS_INTERNAL_ERROR;
        }
        return STATUS_OK;
    }

    if (result == ENGINE_OK)
    {
        log_status = log_event_with_options("INFO", "Scenario evaluated: OK", use_color);
    }
    else if (result == ENGINE_WARNING)
    {
        log_status = log_event_with_options("WARN", "Scenario evaluated: WARNING", use_color);
    }
    else
    {
        log_status = log_event_with_options("ERROR", "Scenario evaluated: SHUTDOWN", use_color);
    }
    if (log_status != STATUS_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    written = snprintf(line,
                       sizeof(line),
                       "Scenario '%s' result: %s\n",
                       name,
                       result_to_display_string(result, use_color));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (hal_shutdown() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_OK;
}

StatusCode run_named_scenario_with_full_options(const char *name,
                                                int32_t show_sim,
                                                int32_t use_color,
                                                int32_t show_control,
                                                int32_t show_state)
{
    return run_named_scenario_with_json(name, show_sim, use_color, show_control, show_state, 0);
}

StatusCode run_named_scenario(const char *name)
{
    return run_named_scenario_with_full_options(name, 0, 0, 0, 0);
}

StatusCode run_named_scenario_with_output(const char *name, int32_t show_sim)
{
    return run_named_scenario_with_full_options(name, show_sim, 0, 0, 0);
}

StatusCode run_named_scenario_with_options(const char *name, int32_t show_sim, int32_t use_color)
{
    return run_named_scenario_with_full_options(name, show_sim, use_color, 0, 0);
}

StatusCode run_scripted_scenario_with_json(const char *script_path,
                                           int32_t show_sim,
                                           int32_t use_color,
                                           int32_t show_control,
                                           int32_t show_state,
                                           int32_t json_output,
                                           int32_t strict_mode)
{
    EngineState engine;
    ScriptScenarioData script_data;
    TickReport tick_reports[SCRIPT_PARSER_MAX_TICKS + 1U];
    unsigned int tick_report_count = 0U;
    StatusCode status;
    int result;
    char error_message[TEST_LINE_BUFFER_SIZE];
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (hal_init() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    if (script_parser_parse_file(script_path,
                                 &script_data,
                                 error_message,
                                 (uint32_t)sizeof(error_message),
                                 strict_mode) != STATUS_OK)
    {
        if (json_output == 0)
        {
            (void)log_event_with_options("ERROR", error_message, use_color);
        }
        (void)hal_shutdown();
        return STATUS_PARSE_ERROR;
    }

    status = engine_reset(&engine);
    if (status != STATUS_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if ((strict_mode != 0) && (script_data.parse_warning_count > 0U))
    {
        (void)hal_shutdown();
        return STATUS_PARSE_ERROR;
    }

    if ((strict_mode == 0) && (script_data.parse_warning_count > 0U) && (json_output == 0))
    {
        if (log_event_with_options("WARN", "Script parse warnings detected and tolerated (non-strict mode)", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }

    result = execute_profile(&engine,
                             script_data.tick_values,
                             script_data.rpm_values,
                             script_data.temp_values,
                             script_data.oil_values,
                             script_data.run_values,
                             script_data.tick_count,
                             (json_output != 0) ? 0 : show_sim,
                             (json_output != 0) ? 0 : show_control,
                             (json_output != 0) ? 0 : show_state,
                             tick_reports,
                             SCRIPT_PARSER_MAX_TICKS + 1U,
                             &tick_report_count);
    if (result == ENGINE_ERROR)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (json_output != 0)
    {
        if (print_json_report_header() != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }

        if (print_json_scenario_object("scripted_scenario",
                                       REQ_ENG_SCRIPT,
                                       tick_reports,
                                       tick_report_count,
                                       result,
                                       result,
                                       1) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }

        if (print_json_report_footer(1, 1) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (hal_shutdown() != STATUS_OK)
        {
            return STATUS_INTERNAL_ERROR;
        }
        return STATUS_OK;
    }

    if (result == ENGINE_OK)
    {
        if (log_event_with_options("INFO", "Scripted scenario evaluated: OK", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }
    else if (result == ENGINE_WARNING)
    {
        if (log_event_with_options("WARN", "Scripted scenario evaluated: WARNING", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        if (log_event_with_options("ERROR", "Scripted scenario evaluated: SHUTDOWN", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }

    written = snprintf(line,
                       sizeof(line),
                       "Script '%s' result: %s\n",
                       script_path,
                       result_to_display_string(result, use_color));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (print_line(line) != ENGINE_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (hal_shutdown() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_OK;
}
