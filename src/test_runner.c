#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "control.h"
#include "engine.h"
#include "logger.h"
#include "test_runner.h"

#define TEST_LINE_BUFFER_SIZE 256
#define RESULT_COLUMN_WIDTH 8
#define MAX_PROFILE_TICKS 26U
#define MAX_SCRIPT_TICKS 64U
#define SCRIPT_LINE_BUFFER_SIZE 192

#define SCRIPT_MIN_RPM 0.0f
#define SCRIPT_MAX_RPM 10000.0f
#define SCRIPT_MIN_TEMP -50.0f
#define SCRIPT_MAX_TEMP 200.0f
#define SCRIPT_MIN_OIL 0.0f
#define SCRIPT_MAX_OIL 10.0f

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
        if (control_status != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }

        written = snprintf(line, sizeof(line), "CTRL | output=%6.2f%%\n", control_output);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (show_state != 0)
    {
        const char *mode_string;

        if (engine_get_mode_string(engine, &mode_string) != ENGINE_OK)
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
                           const unsigned int *tick_values,
                           const float *rpm_values,
                           const float *temp_values,
                           const float *oil_values,
                           const int *run_values,
                           unsigned int tick_count,
                           int show_sim,
                           int show_control,
                           int show_state,
                           TickReport *tick_reports,
                           unsigned int tick_report_capacity,
                           unsigned int *tick_report_count)
{
    unsigned int tick_index;
    int status;
    int result = ENGINE_OK;

    if ((engine == (EngineState *)0) || (rpm_values == (const float *)0) ||
        (temp_values == (const float *)0) || (oil_values == (const float *)0))
    {
        return ENGINE_ERROR;
    }

    if ((tick_count == 0U) || (tick_count > MAX_SCRIPT_TICKS))
    {
        return ENGINE_ERROR;
    }

    if (tick_report_count != (unsigned int *)0)
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
    if (status != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    for (tick_index = 0U; tick_index < tick_count; ++tick_index)
    {
        int run_flag;

        if (run_values == (const int *)0)
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

        engine->is_running = run_flag;
        engine->rpm = rpm_values[tick_index];
        engine->temperature = temp_values[tick_index];
        engine->oil_pressure = oil_values[tick_index];

        result = evaluate_engine(engine);
        if (result == ENGINE_ERROR)
        {
            return ENGINE_ERROR;
        }

        if (tick_reports != (TickReport *)0)
        {
            float control_output = 0.0f;
            unsigned int report_index = tick_index + 1U; /* +1: index 0 is the pre-start INIT tick */

            if (compute_control_output(engine, &control_output) != ENGINE_OK)
            {
                return ENGINE_ERROR;
            }

            tick_reports[report_index].tick =
                (tick_values == (const unsigned int *)0) ? (tick_index + 1U) : tick_values[tick_index];
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
            unsigned int tick_value;

            tick_value = (tick_values == (const unsigned int *)0) ? (tick_index + 1U) : tick_values[tick_index];
            if (print_tick_details(tick_value, engine, result, show_sim, show_control, show_state) != ENGINE_OK)
            {
                return ENGINE_ERROR;
            }
        }

        status = engine_update(engine);
        if (status != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (tick_report_count != (unsigned int *)0)
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

static int print_json_scenario_object(const char *scenario_name,
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

    if ((scenario_name == (const char *)0) || (tick_reports == (const TickReport *)0))
    {
        return ENGINE_ERROR;
    }

    expected_value = has_expected != 0 ? expected_result : actual_result;
    is_pass = (expected_value == actual_result) ? 1 : 0;

    if (print_line("    {\n") != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"scenario\": \"%s\",\n", scenario_name);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    if (print_line("      \"ticks\": [\n") != ENGINE_OK)
    {
        return ENGINE_ERROR;
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
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (print_line("      ],\n") != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"expected\": \"%s\",\n", result_to_string(expected_value));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"actual\": \"%s\",\n", result_to_string(actual_result));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"pass\": %s\n", (is_pass != 0) ? "true" : "false");
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    return print_line("    }");
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
    int status;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;
    TickReport tick_reports[MAX_PROFILE_TICKS];
    unsigned int tick_report_count = 0U;
    int local_show_sim;
    int local_show_control;
    int local_show_state;

    if ((test_case == (const TestCase *)0) || (test_case->name == (const char *)0) ||
        (test_case->scenario_func == (int (*)(EngineState *, int, int, int, void *, unsigned int, unsigned int *))0))
    {
        return 0;
    }

    status = engine_reset(&engine);
    if (status != ENGINE_OK)
    {
        return 0;
    }

    if (json_output == 0)
    {
        if (print_test_separator() != ENGINE_OK)
        {
            return 0;
        }

        written = snprintf(line, sizeof(line), "TEST | %s\n", test_case->name);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return 0;
        }
        if (print_line(line) != ENGINE_OK)
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

        if (print_json_scenario_object(test_case->name,
                                       tick_reports,
                                       tick_report_count,
                                       test_case->expected_result,
                                       actual_result,
                                       1) != ENGINE_OK)
        {
            return 0;
        }
    }
    else
    {
        written = snprintf(line,
                           sizeof(line),
                           "EVAL | expected=%-*s  actual=%-*s  => %s\n",
                           RESULT_COLUMN_WIDTH,
                           result_to_display_string(test_case->expected_result, use_color),
                           RESULT_COLUMN_WIDTH,
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

static int parse_script_line(const char *line,
                             unsigned int *tick,
                             float *rpm,
                             float *temp,
                             float *oil,
                             int *run)
{
    char key_tick[8];
    char key_rpm[8];
    char key_temp[8];
    char key_oil[8];
    char key_run[8];
    int consumed = 0;
    int parsed;
    unsigned int tick_value;
    float rpm_value;
    float temp_value;
    float oil_value;
    int run_value;
    const char *remaining;

    if ((line == (const char *)0) || (tick == (unsigned int *)0) || (rpm == (float *)0) || (temp == (float *)0) ||
        (oil == (float *)0) || (run == (int *)0))
    {
        return ENGINE_ERROR;
    }

    parsed = sscanf(line,
                    " %7s %u %7s %f %7s %f %7s %f %7s %d %n",
                    key_tick,
                    &tick_value,
                    key_rpm,
                    &rpm_value,
                    key_temp,
                    &temp_value,
                    key_oil,
                    &oil_value,
                    key_run,
                    &run_value,
                    &consumed);

    if (parsed != 10)
    {
        return ENGINE_ERROR;
    }

    if ((strcmp(key_tick, "TICK") != 0) || (strcmp(key_rpm, "RPM") != 0) || (strcmp(key_temp, "TEMP") != 0) ||
        (strcmp(key_oil, "OIL") != 0) || (strcmp(key_run, "RUN") != 0))
    {
        return ENGINE_ERROR;
    }

    remaining = line + consumed;
    while (*remaining != '\0')
    {
        if (!isspace((unsigned char)*remaining))
        {
            return ENGINE_ERROR;
        }
        ++remaining;
    }

    if ((tick_value == 0U) || (rpm_value < SCRIPT_MIN_RPM) || (rpm_value > SCRIPT_MAX_RPM) ||
        (temp_value < SCRIPT_MIN_TEMP) || (temp_value > SCRIPT_MAX_TEMP) || (oil_value < SCRIPT_MIN_OIL) ||
        (oil_value > SCRIPT_MAX_OIL) || ((run_value != 0) && (run_value != 1)))
    {
        return ENGINE_ERROR;
    }

    *tick = tick_value;
    *rpm = rpm_value;
    *temp = temp_value;
    *oil = oil_value;
    *run = run_value;

    return ENGINE_OK;
}

static int line_is_whitespace_only(const char *line)
{
    const char *cursor;

    if (line == (const char *)0)
    {
        return 1;
    }

    cursor = line;
    while (*cursor != '\0')
    {
        if (!isspace((unsigned char)*cursor))
        {
            return 0;
        }
        ++cursor;
    }

    return 1;
}

static int parse_script_file(const char *script_path,
                             unsigned int *tick_values,
                             float *rpm_values,
                             float *temp_values,
                             float *oil_values,
                             int *run_values,
                             unsigned int *tick_count,
                             char *error_message,
                             unsigned int error_message_size)
{
    FILE *script_file;
    char line_buffer[SCRIPT_LINE_BUFFER_SIZE];
    unsigned int line_number = 0U;
    unsigned int parsed_ticks = 0U;

    if ((script_path == (const char *)0) || (tick_values == (unsigned int *)0) || (rpm_values == (float *)0) ||
        (temp_values == (float *)0) || (oil_values == (float *)0) || (run_values == (int *)0) ||
        (tick_count == (unsigned int *)0) || (error_message == (char *)0) || (error_message_size == 0U))
    {
        return ENGINE_ERROR;
    }

    *tick_count = 0U;
    error_message[0] = '\0';

    script_file = fopen(script_path, "r");
    if (script_file == (FILE *)0)
    {
        (void)snprintf(error_message, error_message_size, "Unable to open script file: %s", script_path);
        return ENGINE_ERROR;
    }

    while (fgets(line_buffer, sizeof(line_buffer), script_file) != (char *)0)
    {
        size_t line_len;

        line_number += 1U;
        line_len = strlen(line_buffer);

        if ((line_len > 0U) && (line_buffer[line_len - 1U] != '\n') && !feof(script_file))
        {
            int ch;
            while (((ch = fgetc(script_file)) != '\n') && (ch != EOF))
            {
                ;
            }
            (void)snprintf(error_message,
                           error_message_size,
                           "Line %u exceeds maximum length of %u characters",
                           line_number,
                           (unsigned int)(SCRIPT_LINE_BUFFER_SIZE - 2U));
            (void)fclose(script_file);
            return ENGINE_ERROR;
        }

        if (line_is_whitespace_only(line_buffer) != 0)
        {
            continue;
        }

        if (parsed_ticks >= MAX_SCRIPT_TICKS)
        {
            (void)snprintf(error_message,
                           error_message_size,
                           "Script tick count exceeds limit (%u)",
                           (unsigned int)MAX_SCRIPT_TICKS);
            (void)fclose(script_file);
            return ENGINE_ERROR;
        }

        if (parse_script_line(line_buffer,
                              &tick_values[parsed_ticks],
                              &rpm_values[parsed_ticks],
                              &temp_values[parsed_ticks],
                              &oil_values[parsed_ticks],
                              &run_values[parsed_ticks]) != ENGINE_OK)
        {
            (void)snprintf(error_message,
                           error_message_size,
                           "Malformed script line %u: expected 'TICK <n> RPM <v> TEMP <v> OIL <v> RUN <0|1>'",
                           line_number);
            (void)fclose(script_file);
            return ENGINE_ERROR;
        }

        if ((parsed_ticks > 0U) && (tick_values[parsed_ticks] <= tick_values[parsed_ticks - 1U]))
        {
            (void)snprintf(error_message,
                           error_message_size,
                           "Line %u has non-increasing tick value (%u)",
                           line_number,
                           tick_values[parsed_ticks]);
            (void)fclose(script_file);
            return ENGINE_ERROR;
        }

        parsed_ticks += 1U;
    }

    if (fclose(script_file) != 0)
    {
        (void)snprintf(error_message, error_message_size, "Failed to close script file: %s", script_path);
        return ENGINE_ERROR;
    }

    if (parsed_ticks == 0U)
    {
        (void)snprintf(error_message, error_message_size, "Script file contains no tick data: %s", script_path);
        return ENGINE_ERROR;
    }

    *tick_count = parsed_ticks;
    return ENGINE_OK;
}

int run_all_tests_with_json(int show_sim, int use_color, int show_control, int show_state, int json_output)
{
    const TestCase tests[MAX_TESTS] = {
        {"normal_operation", scenario_normal_operation, ENGINE_OK},
        {"overheat_lt_persistence", scenario_overheat_short_duration, ENGINE_OK},
        {"overheat_ge_persistence", scenario_overheat_persistent, ENGINE_SHUTDOWN},
        {"oil_low_ge_persistence", scenario_oil_pressure_persistent, ENGINE_SHUTDOWN},
        {"combined_warning_persistence", scenario_combined_warning_persistent, ENGINE_WARNING},
        {"cold_start", scenario_cold_start_warmup_and_ramp, ENGINE_OK},
        {"high_load", scenario_high_load_warning_then_recovery, ENGINE_WARNING},
        {"oil_drain", scenario_oil_pressure_gradual_drain, ENGINE_SHUTDOWN},
        {"thermal_runaway", scenario_thermal_runaway_with_load_surge, ENGINE_SHUTDOWN},
        {"intermittent_oil", scenario_intermittent_oil_then_combined_fault, ENGINE_WARNING}};
    int passed = 0;
    const int total = MAX_TESTS;
    int index;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (json_output == 0)
    {
        if (log_event_with_options("INFO", "Running automated validation tests", use_color) != 0)
        {
            return 1;
        }
    }
    else
    {
        if (print_line("{\n  \"scenarios\": [\n") != ENGINE_OK)
        {
            return 1;
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
        if (print_line("\n  ],\n") != ENGINE_OK)
        {
            return 1;
        }

        written = snprintf(line,
                           sizeof(line),
                           "  \"summary\": {\"passed\": %d, \"total\": %d}\n}\n",
                           passed,
                           total);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return 1;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return 1;
        }
    }
    else
    {
        if (print_test_separator() != ENGINE_OK)
        {
            return 1;
        }

        written = snprintf(line, sizeof(line), "Summary: %d/%d tests passed\n", passed, total);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return 1;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return 1;
        }

        if (passed == total)
        {
            if (log_event_with_options("INFO", "All tests passed", use_color) != 0)
            {
                return 1;
            }
            return 0;
        }

        if (log_event_with_options("ERROR", "One or more tests failed", use_color) != 0)
        {
            return 1;
        }
    }

    return (passed == total) ? 0 : 1;
}

int run_all_tests_with_full_options(int show_sim, int use_color, int show_control, int show_state)
{
    return run_all_tests_with_json(show_sim, use_color, show_control, show_state, 0);
}

int run_all_tests(void)
{
    return run_all_tests_with_full_options(0, 0, 0, 0);
}

int run_all_tests_with_output(int show_sim)
{
    return run_all_tests_with_full_options(show_sim, 0, 0, 0);
}

int run_all_tests_with_options(int show_sim, int use_color)
{
    return run_all_tests_with_full_options(show_sim, use_color, 0, 0);
}

int run_named_scenario_with_json(const char *name,
                                 int show_sim,
                                 int use_color,
                                 int show_control,
                                 int show_state,
                                 int json_output)
{
    EngineState engine;
    int result;
    int status;
    int log_status;
    int expected_result;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;
    TickReport tick_reports[MAX_PROFILE_TICKS];
    unsigned int tick_report_count = 0U;
    int local_show_sim;
    int local_show_control;
    int local_show_state;

    if (valid_scenario_name(name) == 0)
    {
        return ENGINE_ERROR;
    }

    status = engine_reset(&engine);
    if (status != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    expected_result = ENGINE_OK;
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
        return ENGINE_ERROR;
    }

    if (result == ENGINE_ERROR)
    {
        return ENGINE_ERROR;
    }

    if (json_output != 0)
    {
        if (print_line("{\n  \"scenarios\": [\n") != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
        if (print_json_scenario_object(name, tick_reports, tick_report_count, expected_result, result, 1) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
        if (print_line("\n  ],\n") != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
        written = snprintf(line,
                           sizeof(line),
                           "  \"summary\": {\"passed\": %d, \"total\": 1}\n}\n",
                           (result == expected_result) ? 1 : 0);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
        return result;
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
    if (log_status != 0)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line,
                       sizeof(line),
                       "Scenario '%s' result: %s\n",
                       name,
                       result_to_display_string(result, use_color));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    return result;
}

int run_named_scenario_with_full_options(const char *name,
                                         int show_sim,
                                         int use_color,
                                         int show_control,
                                         int show_state)
{
    return run_named_scenario_with_json(name, show_sim, use_color, show_control, show_state, 0);
}

int run_named_scenario(const char *name)
{
    return run_named_scenario_with_full_options(name, 0, 0, 0, 0);
}

int run_named_scenario_with_output(const char *name, int show_sim)
{
    return run_named_scenario_with_full_options(name, show_sim, 0, 0, 0);
}

int run_named_scenario_with_options(const char *name, int show_sim, int use_color)
{
    return run_named_scenario_with_full_options(name, show_sim, use_color, 0, 0);
}

int run_scripted_scenario_with_json(const char *script_path,
                                    int show_sim,
                                    int use_color,
                                    int show_control,
                                    int show_state,
                                    int json_output)
{
    EngineState engine;
    unsigned int tick_values[MAX_SCRIPT_TICKS];
    float rpm_values[MAX_SCRIPT_TICKS];
    float temp_values[MAX_SCRIPT_TICKS];
    float oil_values[MAX_SCRIPT_TICKS];
    int run_values[MAX_SCRIPT_TICKS];
    unsigned int tick_count = 0U;
    TickReport tick_reports[MAX_SCRIPT_TICKS + 1U];
    unsigned int tick_report_count = 0U;
    int status;
    int result;
    char error_message[TEST_LINE_BUFFER_SIZE];
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (parse_script_file(script_path,
                          tick_values,
                          rpm_values,
                          temp_values,
                          oil_values,
                          run_values,
                          &tick_count,
                          error_message,
                          (unsigned int)sizeof(error_message)) != ENGINE_OK)
    {
        if (json_output == 0)
        {
            (void)log_event_with_options("ERROR", error_message, use_color);
        }
        return ENGINE_ERROR;
    }

    status = engine_reset(&engine);
    if (status != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    result = execute_profile(&engine,
                             tick_values,
                             rpm_values,
                             temp_values,
                             oil_values,
                             run_values,
                             tick_count,
                             (json_output != 0) ? 0 : show_sim,
                             (json_output != 0) ? 0 : show_control,
                             (json_output != 0) ? 0 : show_state,
                             tick_reports,
                             MAX_SCRIPT_TICKS + 1U,
                             &tick_report_count);
    if (result == ENGINE_ERROR)
    {
        return ENGINE_ERROR;
    }

    if (json_output != 0)
    {
        if (print_line("{\n  \"scenarios\": [\n") != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }

        if (print_json_scenario_object("scripted_scenario", tick_reports, tick_report_count, result, result, 1) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }

        if (print_line("\n  ],\n") != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }

        written = snprintf(line, sizeof(line), "  \"summary\": {\"passed\": 1, \"total\": 1}\n}\n");
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
        return result;
    }

    if (result == ENGINE_OK)
    {
        if (log_event_with_options("INFO", "Scripted scenario evaluated: OK", use_color) != 0)
        {
            return ENGINE_ERROR;
        }
    }
    else if (result == ENGINE_WARNING)
    {
        if (log_event_with_options("WARN", "Scripted scenario evaluated: WARNING", use_color) != 0)
        {
            return ENGINE_ERROR;
        }
    }
    else
    {
        if (log_event_with_options("ERROR", "Scripted scenario evaluated: SHUTDOWN", use_color) != 0)
        {
            return ENGINE_ERROR;
        }
    }

    written = snprintf(line,
                       sizeof(line),
                       "Script '%s' result: %s\n",
                       script_path,
                       result_to_display_string(result, use_color));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }

    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    return result;
}
