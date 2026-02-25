#include <stdio.h>

#include "control.h"
#include "engine.h"
#include "hal.h"
#include "reporting/output.h"
#include "scenario/scenario_report.h"
#include "reporting/version.h"

#define TEST_LINE_BUFFER_SIZE 256

#define ANSI_RESET "\x1b[0m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"

const char *scenario_report_result_to_string(int32_t result)
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

const char *scenario_report_mode_to_string(EngineStateMode mode)
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

const char *scenario_report_result_to_display_string(int32_t result, int32_t use_color)
{
    if (use_color == 0)
    {
        return scenario_report_result_to_string(result);
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

const char *scenario_report_pass_fail_display(int32_t is_pass, int32_t use_color)
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

int32_t scenario_report_print_tick_details(uint32_t tick,
                                           const EngineState *engine,
                                           int32_t result,
                                           int32_t show_sim,
                                           int32_t show_control,
                                           int32_t show_state)
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
                       scenario_report_result_to_string(result));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (output_write_line(line) != ENGINE_OK)
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
        if (output_write_line(line) != ENGINE_OK)
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
        if (output_write_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    return ENGINE_OK;
}

StatusCode scenario_report_print_json_scenario_object(const char *scenario_name,
                                                      const char *requirement_id,
                                                      const TickReport *tick_reports,
                                                      uint32_t tick_count,
                                                      int32_t expected_result,
                                                      int32_t actual_result,
                                                      int32_t has_expected)
{
    char line[TEST_LINE_BUFFER_SIZE];
    int written;
    uint32_t index;
    int32_t expected_value;
    int32_t is_pass;

    if ((scenario_name == (const char *)0) || (requirement_id == (const char *)0) ||
        (tick_reports == (const TickReport *)0))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    expected_value = has_expected != 0 ? expected_result : actual_result;
    is_pass = (expected_value == actual_result) ? 1 : 0;

    if (output_write_line("    {\n") != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"scenario\": \"%s\",\n", scenario_name);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (output_write_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"requirement_id\": \"%s\",\n", requirement_id);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (output_write_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    if (output_write_line("      \"ticks\": [\n") != ENGINE_OK)
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
                           scenario_report_result_to_string(tick_reports[index].result),
                           tick_reports[index].control,
                           scenario_report_mode_to_string(tick_reports[index].mode),
                           (index + 1U < tick_count) ? "," : "");
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return STATUS_BUFFER_OVERFLOW;
        }
        if (output_write_line(line) != ENGINE_OK)
        {
            return STATUS_IO_ERROR;
        }
    }

    if (output_write_line("      ],\n") != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"expected\": \"%s\",\n", scenario_report_result_to_string(expected_value));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (output_write_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"actual\": \"%s\",\n", scenario_report_result_to_string(actual_result));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (output_write_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "      \"pass\": %s\n", (is_pass != 0) ? "true" : "false");
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (output_write_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    return (output_write_line("    }") == ENGINE_OK) ? STATUS_OK : STATUS_IO_ERROR;
}

StatusCode scenario_report_print_json_header(void)
{
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (output_write_line("{\n") != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "  \"schema_version\": \"%s\",\n", SIM_SCHEMA_VERSION);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (output_write_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    written = snprintf(line, sizeof(line), "  \"software_version\": \"%s\",\n", SIM_SOFTWARE_VERSION);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    if (output_write_line(line) != ENGINE_OK)
    {
        return STATUS_IO_ERROR;
    }

    return (output_write_line("  \"scenarios\": [\n") == ENGINE_OK) ? STATUS_OK : STATUS_IO_ERROR;
}

StatusCode scenario_report_print_json_footer(int32_t passed, int32_t total)
{
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (output_write_line("\n  ],\n") != ENGINE_OK)
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

    return (output_write_line(line) == ENGINE_OK) ? STATUS_OK : STATUS_IO_ERROR;
}
