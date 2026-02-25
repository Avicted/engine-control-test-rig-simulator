#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "script_parser.h"

#define SCRIPT_LINE_BUFFER_SIZE 192

#define SCRIPT_MIN_RPM 0.0f
#define SCRIPT_MAX_RPM 10000.0f
#define SCRIPT_MIN_TEMP -50.0f
#define SCRIPT_MAX_TEMP 200.0f
#define SCRIPT_MIN_OIL 0.0f
#define SCRIPT_MAX_OIL 10.0f

static int32_t parse_strict_uint(const char *token, uint32_t *value)
{
    unsigned long parsed;
    char *endptr;

    if ((token == (const char *)0) || (value == (uint32_t *)0))
    {
        return 0;
    }

    if ((*token == '\0') || (*token == '-') || (*token == '+'))
    {
        return 0;
    }

    errno = 0;
    parsed = strtoul(token, &endptr, 10);
    if ((errno != 0) || (endptr == token) || (*endptr != '\0') || (parsed == 0UL) || (parsed > UINT_MAX))
    {
        return 0;
    }

    *value = (uint32_t)parsed;
    return 1;
}

static int32_t parse_strict_float(const char *token, float *value)
{
    float parsed;
    char *endptr;

    if ((token == (const char *)0) || (value == (float *)0) || (*token == '\0'))
    {
        return 0;
    }

    errno = 0;
    parsed = strtof(token, &endptr);
    if ((errno != 0) || (endptr == token) || (*endptr != '\0') || !isfinite(parsed))
    {
        return 0;
    }

    *value = parsed;
    return 1;
}

static StatusCode parse_script_line(const char *line,
                                    uint32_t *tick,
                                    float *rpm,
                                    float *temp,
                                    float *oil,
                                    int32_t *run)
{
    char key_tick[8];
    char token_tick[32];
    char key_rpm[8];
    char token_rpm[32];
    char key_temp[8];
    char token_temp[32];
    char key_oil[8];
    char token_oil[32];
    char key_run[8];
    char token_run[8];
    int parsed;
    uint32_t tick_value;
    float rpm_value;
    float temp_value;
    float oil_value;
    int32_t run_value;

    if ((line == (const char *)0) || (tick == (uint32_t *)0) || (rpm == (float *)0) || (temp == (float *)0) ||
        (oil == (float *)0) || (run == (int32_t *)0))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    parsed = sscanf(line,
                    " %7s %31s %7s %31s %7s %31s %7s %31s %7s %7s",
                    key_tick,
                    token_tick,
                    key_rpm,
                    token_rpm,
                    key_temp,
                    token_temp,
                    key_oil,
                    token_oil,
                    key_run,
                    token_run);

    if (parsed != 10)
    {
        return STATUS_PARSE_ERROR;
    }

    if ((strcmp(key_tick, "TICK") != 0) || (strcmp(key_rpm, "RPM") != 0) || (strcmp(key_temp, "TEMP") != 0) ||
        (strcmp(key_oil, "OIL") != 0) || (strcmp(key_run, "RUN") != 0))
    {
        return STATUS_PARSE_ERROR;
    }

    if ((parse_strict_uint(token_tick, &tick_value) == 0) || (parse_strict_float(token_rpm, &rpm_value) == 0) ||
        (parse_strict_float(token_temp, &temp_value) == 0) || (parse_strict_float(token_oil, &oil_value) == 0))
    {
        return STATUS_PARSE_ERROR;
    }

    if (strcmp(token_run, "0") == 0)
    {
        run_value = 0;
    }
    else if (strcmp(token_run, "1") == 0)
    {
        run_value = 1;
    }
    else
    {
        return STATUS_PARSE_ERROR;
    }

    if ((rpm_value < SCRIPT_MIN_RPM) || (rpm_value > SCRIPT_MAX_RPM) ||
        (temp_value < SCRIPT_MIN_TEMP) || (temp_value > SCRIPT_MAX_TEMP) || (oil_value < SCRIPT_MIN_OIL) ||
        (oil_value > SCRIPT_MAX_OIL) || ((run_value != 0) && (run_value != 1)))
    {
        return STATUS_PARSE_ERROR;
    }

    *tick = tick_value;
    *rpm = rpm_value;
    *temp = temp_value;
    *oil = oil_value;
    *run = run_value;

    return STATUS_OK;
}

static int32_t line_is_whitespace_only(const char *line)
{
    const unsigned char *cursor;

    if (line == (const char *)0)
    {
        return 1;
    }

    cursor = (const unsigned char *)line;
    while (*cursor != '\0')
    {
        if ((*cursor != ' ') && (*cursor != '\t') && (*cursor != '\n') && (*cursor != '\r') &&
            (*cursor != '\v') && (*cursor != '\f'))
        {
            return 0;
        }
        ++cursor;
    }

    return 1;
}

static int32_t line_is_comment(const char *line)
{
    const unsigned char *cursor;

    if (line == (const char *)0)
    {
        return 0;
    }

    cursor = (const unsigned char *)line;
    while ((*cursor == ' ') || (*cursor == '\t') || (*cursor == '\r') || (*cursor == '\n') ||
           (*cursor == '\v') || (*cursor == '\f'))
    {
        ++cursor;
    }

    return (*cursor == '#') ? 1 : 0;
}

StatusCode script_parser_parse_file(const char *script_path,
                                    ScriptScenarioData *scenario_data,
                                    char *error_message,
                                    uint32_t error_message_size,
                                    int32_t strict_mode)
{
    FILE *script_file;
    char line_buffer[SCRIPT_LINE_BUFFER_SIZE];
    uint32_t line_number = 0U;
    uint32_t parsed_ticks = 0U;
    float rpm_value = 0.0f;
    float temp_value = 0.0f;
    float oil_value = 0.0f;
    int32_t run_value = 0;

    if ((script_path == (const char *)0) || (scenario_data == (ScriptScenarioData *)0) || (error_message == (char *)0) ||
        (error_message_size == 0U))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    scenario_data->tick_count = 0U;
    scenario_data->parse_warning_count = 0U;
    error_message[0] = '\0';

    script_file = fopen(script_path, "r");
    if (script_file == (FILE *)0)
    {
        (void)snprintf(error_message, error_message_size, "Unable to open script file: %s", script_path);
        return STATUS_IO_ERROR;
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
                           (uint32_t)(SCRIPT_LINE_BUFFER_SIZE - 2U));
            (void)fclose(script_file);
            return STATUS_PARSE_ERROR;
        }

        if (line_is_whitespace_only(line_buffer) != 0)
        {
            continue;
        }

        if (line_is_comment(line_buffer) != 0)
        {
            if (strict_mode != 0)
            {
                (void)snprintf(error_message,
                               error_message_size,
                               "Strict mode parse warning at line %u: comments are not allowed",
                               line_number);
                (void)fclose(script_file);
                return STATUS_PARSE_ERROR;
            }

            scenario_data->parse_warning_count += 1U;
            continue;
        }

        if (parsed_ticks >= SCRIPT_PARSER_MAX_TICKS)
        {
            (void)snprintf(error_message,
                           error_message_size,
                           "Script tick count exceeds limit (%u)",
                           SCRIPT_PARSER_MAX_TICKS);
            (void)fclose(script_file);
            return STATUS_PARSE_ERROR;
        }

        if (parse_script_line(line_buffer,
                              &scenario_data->tick_values[parsed_ticks],
                              &rpm_value,
                              &temp_value,
                              &oil_value,
                              &run_value) != STATUS_OK)
        {
            (void)snprintf(error_message,
                           error_message_size,
                           "Malformed script line %u: expected 'TICK <n> RPM <v> TEMP <v> OIL <v> RUN <0|1>'",
                           line_number);
            (void)fclose(script_file);
            return STATUS_PARSE_ERROR;
        }

        {
            HAL_SensorFrame sensor_frame;

            sensor_frame.rpm = rpm_value;
            sensor_frame.temperature = temp_value;
            sensor_frame.oil_pressure = oil_value;
            sensor_frame.is_running = run_value;
            if (hal_encode_sensor_frame(&sensor_frame, &scenario_data->sensor_frames[parsed_ticks]) != STATUS_OK)
            {
                (void)snprintf(error_message,
                               error_message_size,
                               "Line %u contains out-of-range sensor values",
                               line_number);
                (void)fclose(script_file);
                return STATUS_PARSE_ERROR;
            }
        }

        if ((parsed_ticks > 0U) &&
            (scenario_data->tick_values[parsed_ticks] <= scenario_data->tick_values[parsed_ticks - 1U]))
        {
            (void)snprintf(error_message,
                           error_message_size,
                           "Line %u has non-increasing tick value (%u)",
                           line_number,
                           scenario_data->tick_values[parsed_ticks]);
            (void)fclose(script_file);
            return STATUS_PARSE_ERROR;
        }

        parsed_ticks += 1U;
    }

    if (fclose(script_file) != 0)
    {
        (void)snprintf(error_message, error_message_size, "Failed to close script file: %s", script_path);
        return STATUS_IO_ERROR;
    }

    if (parsed_ticks == 0U)
    {
        (void)snprintf(error_message, error_message_size, "Script file contains no tick data: %s", script_path);
        return STATUS_PARSE_ERROR;
    }

    scenario_data->tick_count = parsed_ticks;
    return STATUS_OK;
}
