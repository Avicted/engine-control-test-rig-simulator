#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "visualizer_loader.h"

#define EXPECTED_SCHEMA_VERSION "1.0"
#define EXPECTED_SCHEMA_VERSION_V2 "1.0.0"

static const char *skip_ws(const char *cursor)
{
    while ((cursor != NULL) && (*cursor != '\0') && isspace((unsigned char)*cursor))
    {
        cursor++;
    }
    return cursor;
}

static const char *find_key(const char *start, const char *key)
{
    const char *cursor;

    if ((start == NULL) || (key == NULL))
    {
        return NULL;
    }

    cursor = strstr(start, key);
    if (cursor == NULL)
    {
        return NULL;
    }

    return cursor;
}

static const char *find_value_start(const char *key_pos)
{
    const char *colon;

    if (key_pos == NULL)
    {
        return NULL;
    }

    colon = strchr(key_pos, ':');
    if (colon == NULL)
    {
        return NULL;
    }

    return skip_ws(colon + 1);
}

static int parse_quoted_value(const char *object_start, const char *key, char *out, size_t out_size)
{
    const char *key_pos;
    const char *value_start;
    const char *value_end;
    size_t length;

    if ((object_start == NULL) || (key == NULL) || (out == NULL) || (out_size < 2U))
    {
        return 0;
    }

    key_pos = find_key(object_start, key);
    if (key_pos == NULL)
    {
        return 0;
    }

    value_start = find_value_start(key_pos);
    if ((value_start == NULL) || (*value_start != '"'))
    {
        return 0;
    }

    value_start++;
    value_end = strchr(value_start, '"');
    if (value_end == NULL)
    {
        return 0;
    }

    length = (size_t)(value_end - value_start);
    if (length >= out_size)
    {
        return 0;
    }

    (void)memcpy(out, value_start, length);
    out[length] = '\0';
    return 1;
}

static int parse_uint_value(const char *object_start, const char *key, unsigned int *out)
{
    const char *key_pos;
    const char *value_start;
    char *endptr;
    unsigned long parsed;

    if ((object_start == NULL) || (key == NULL) || (out == NULL))
    {
        return 0;
    }

    key_pos = find_key(object_start, key);
    if (key_pos == NULL)
    {
        return 0;
    }

    value_start = find_value_start(key_pos);
    if (value_start == NULL)
    {
        return 0;
    }

    if ((*value_start == '-') || (*value_start == '+'))
    {
        return 0;
    }

    errno = 0;
    parsed = strtoul(value_start, &endptr, 10);
    if ((errno != 0) || (endptr == value_start) || ((parsed == ULONG_MAX) && (errno != 0)) ||
        (parsed > UINT_MAX))
    {
        return 0;
    }

    endptr = (char *)skip_ws(endptr);
    if ((*endptr != ',') && (*endptr != '}') && (*endptr != ']') && (*endptr != '\0'))
    {
        return 0;
    }

    *out = (unsigned int)parsed;
    return 1;
}

static int parse_int_value(const char *object_start, const char *key, int *out)
{
    const char *key_pos;
    const char *value_start;
    char *endptr;
    long parsed;

    if ((object_start == NULL) || (key == NULL) || (out == NULL))
    {
        return 0;
    }

    key_pos = find_key(object_start, key);
    if (key_pos == NULL)
    {
        return 0;
    }

    value_start = find_value_start(key_pos);
    if (value_start == NULL)
    {
        return 0;
    }

    errno = 0;
    parsed = strtol(value_start, &endptr, 10);
    if ((errno != 0) || (endptr == value_start) || (parsed < INT_MIN) || (parsed > INT_MAX))
    {
        return 0;
    }

    endptr = (char *)skip_ws(endptr);
    if ((*endptr != ',') && (*endptr != '}') && (*endptr != ']') && (*endptr != '\0'))
    {
        return 0;
    }

    *out = (int)parsed;
    return 1;
}

static int parse_float_value(const char *object_start, const char *key, float *out)
{
    const char *key_pos;
    const char *value_start;
    char *endptr;
    float parsed;

    if ((object_start == NULL) || (key == NULL) || (out == NULL))
    {
        return 0;
    }

    key_pos = find_key(object_start, key);
    if (key_pos == NULL)
    {
        return 0;
    }

    value_start = find_value_start(key_pos);
    if (value_start == NULL)
    {
        return 0;
    }

    errno = 0;
    parsed = strtof(value_start, &endptr);
    if ((errno != 0) || (endptr == value_start) || !isfinite(parsed))
    {
        return 0;
    }

    endptr = (char *)skip_ws(endptr);
    if ((*endptr != ',') && (*endptr != '}') && (*endptr != ']') && (*endptr != '\0'))
    {
        return 0;
    }

    *out = parsed;
    return 1;
}

static const char *find_matching_brace(const char *start)
{
    int depth;
    const char *cursor;

    if ((start == NULL) || (*start != '{'))
    {
        return NULL;
    }

    depth = 0;
    cursor = start;

    while (*cursor != '\0')
    {
        if (*cursor == '{')
        {
            depth++;
        }
        else if (*cursor == '}')
        {
            depth--;
            if (depth == 0)
            {
                return cursor;
            }
        }
        cursor++;
    }

    return NULL;
}

static const char *find_matching_bracket(const char *start)
{
    int depth;
    const char *cursor;

    if ((start == NULL) || (*start != '['))
    {
        return NULL;
    }

    depth = 0;
    cursor = start;

    while (*cursor != '\0')
    {
        if (*cursor == '[')
        {
            depth++;
        }
        else if (*cursor == ']')
        {
            depth--;
            if (depth == 0)
            {
                return cursor;
            }
        }
        cursor++;
    }

    return NULL;
}

static int validate_tick(const TickData *tick)
{
    if (tick == NULL)
    {
        return 0;
    }

    if ((tick->rpm < 0.0f) || (tick->rpm > MAX_RPM))
    {
        return 0;
    }
    if ((tick->temp < MIN_TEMP) || (tick->temp > MAX_TEMP))
    {
        return 0;
    }
    if ((tick->oil < MIN_OIL) || (tick->oil > MAX_OIL))
    {
        return 0;
    }
    if ((tick->control < MIN_CONTROL) || (tick->control > MAX_CONTROL))
    {
        return 0;
    }
    if ((tick->run != 0) && (tick->run != 1))
    {
        return 0;
    }

    return 1;
}

static int parse_tick_object(const char *object_start, TickData *tick)
{
    if ((object_start == NULL) || (tick == NULL))
    {
        return 0;
    }

    if (parse_uint_value(object_start, "\"tick\"", &tick->tick) == 0)
    {
        return 0;
    }
    if (parse_float_value(object_start, "\"rpm\"", &tick->rpm) == 0)
    {
        return 0;
    }
    if (parse_float_value(object_start, "\"temp\"", &tick->temp) == 0)
    {
        return 0;
    }
    if (parse_float_value(object_start, "\"oil\"", &tick->oil) == 0)
    {
        return 0;
    }
    if (parse_int_value(object_start, "\"run\"", &tick->run) == 0)
    {
        return 0;
    }
    if (parse_quoted_value(object_start, "\"result\"", tick->result, sizeof(tick->result)) == 0)
    {
        return 0;
    }
    if (parse_float_value(object_start, "\"control\"", &tick->control) == 0)
    {
        return 0;
    }
    if (parse_quoted_value(object_start, "\"engine_mode\"", tick->engine_mode, sizeof(tick->engine_mode)) == 0)
    {
        return 0;
    }

    return validate_tick(tick);
}

static int load_file_text(const char *path, char **buffer_out)
{
    FILE *file;
    long length;
    size_t bytes_read;
    char *buffer;

    if ((path == NULL) || (buffer_out == NULL))
    {
        return 0;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return 0;
    }

    if (fseek(file, 0L, SEEK_END) != 0)
    {
        (void)fclose(file);
        return 0;
    }

    length = ftell(file);
    if ((length <= 0L) || ((unsigned long)length > MAX_FILE_SIZE))
    {
        (void)fclose(file);
        return 0;
    }

    if (fseek(file, 0L, SEEK_SET) != 0)
    {
        (void)fclose(file);
        return 0;
    }

    buffer = (char *)malloc((size_t)length + 1U);
    if (buffer == NULL)
    {
        (void)fclose(file);
        return 0;
    }

    bytes_read = fread(buffer, 1U, (size_t)length, file);
    if ((long)bytes_read != length)
    {
        free(buffer);
        (void)fclose(file);
        return 0;
    }

    buffer[bytes_read] = '\0';
    *buffer_out = buffer;

    if (fclose(file) != 0)
    {
        free(buffer);
        *buffer_out = NULL;
        return 0;
    }

    return 1;
}

static int parse_scenarios_json(const char *json_text,
                                ScenarioData *scenarios,
                                unsigned int max_scenarios,
                                unsigned int *parsed_count)
{
    char schema_version[16];
    char software_version[32];
    const char *scenarios_key;
    const char *array_start;
    const char *cursor;
    const char *array_end;
    unsigned int scenario_count;

    if ((json_text == NULL) || (scenarios == NULL) || (parsed_count == NULL) || (max_scenarios == 0U))
    {
        return 0;
    }

    if (parse_quoted_value(json_text, "\"schema_version\"", schema_version, sizeof(schema_version)) == 0)
    {
        return 0;
    }

    if ((strcmp(schema_version, EXPECTED_SCHEMA_VERSION) != 0) &&
        (strcmp(schema_version, EXPECTED_SCHEMA_VERSION_V2) != 0))
    {
        return 0;
    }

    if (parse_quoted_value(json_text, "\"software_version\"", software_version, sizeof(software_version)) == 0)
    {
        return 0;
    }

    if (software_version[0] == '\0')
    {
        return 0;
    }

    scenarios_key = strstr(json_text, "\"scenarios\"");
    if (scenarios_key == NULL)
    {
        return 0;
    }

    array_start = strchr(scenarios_key, '[');
    if (array_start == NULL)
    {
        return 0;
    }

    array_end = find_matching_bracket(array_start);
    if (array_end == NULL)
    {
        return 0;
    }

    scenario_count = 0U;
    cursor = array_start;

    while (cursor < array_end)
    {
        const char *scenario_object_start;
        const char *scenario_object_end;
        const char *ticks_key;
        const char *ticks_array_start;
        const char *ticks_array_end;
        const char *tick_cursor;

        scenario_object_start = strchr(cursor, '{');
        if ((scenario_object_start == NULL) || (scenario_object_start >= array_end))
        {
            break;
        }

        scenario_object_end = find_matching_brace(scenario_object_start);
        if ((scenario_object_end == NULL) || (scenario_object_end > array_end))
        {
            return 0;
        }

        if (scenario_count >= max_scenarios)
        {
            return 0;
        }

        if (parse_quoted_value(scenario_object_start,
                               "\"scenario\"",
                               scenarios[scenario_count].scenario,
                               sizeof(scenarios[scenario_count].scenario)) == 0)
        {
            return 0;
        }

        if (parse_quoted_value(scenario_object_start,
                               "\"requirement_id\"",
                               scenarios[scenario_count].requirement_id,
                               sizeof(scenarios[scenario_count].requirement_id)) == 0)
        {
            return 0;
        }

        if (parse_quoted_value(scenario_object_start,
                               "\"expected\"",
                               scenarios[scenario_count].expected,
                               sizeof(scenarios[scenario_count].expected)) == 0)
        {
            return 0;
        }

        ticks_key = strstr(scenario_object_start, "\"ticks\"");
        if ((ticks_key == NULL) || (ticks_key > scenario_object_end))
        {
            return 0;
        }

        ticks_array_start = strchr(ticks_key, '[');
        if ((ticks_array_start == NULL) || (ticks_array_start > scenario_object_end))
        {
            return 0;
        }

        ticks_array_end = find_matching_bracket(ticks_array_start);
        if ((ticks_array_end == NULL) || (ticks_array_end > scenario_object_end))
        {
            return 0;
        }

        scenarios[scenario_count].tick_count = 0U;
        tick_cursor = ticks_array_start;
        while (tick_cursor < ticks_array_end)
        {
            const char *obj_start;
            const char *obj_end;

            obj_start = strchr(tick_cursor, '{');
            if ((obj_start == NULL) || (obj_start >= ticks_array_end))
            {
                break;
            }

            obj_end = find_matching_brace(obj_start);
            if ((obj_end == NULL) || (obj_end > ticks_array_end))
            {
                return 0;
            }

            if (scenarios[scenario_count].tick_count >= MAX_TICKS)
            {
                return 0;
            }

            if (parse_tick_object(obj_start,
                                  &scenarios[scenario_count].ticks[scenarios[scenario_count].tick_count]) == 0)
            {
                return 0;
            }

            if ((scenarios[scenario_count].tick_count > 0U) &&
                (scenarios[scenario_count].ticks[scenarios[scenario_count].tick_count].tick <=
                 scenarios[scenario_count].ticks[scenarios[scenario_count].tick_count - 1U].tick))
            {
                return 0;
            }

            scenarios[scenario_count].tick_count++;
            tick_cursor = obj_end + 1;
        }

        if (scenarios[scenario_count].tick_count == 0U)
        {
            return 0;
        }

        scenario_count++;
        cursor = scenario_object_end + 1;
    }

    *parsed_count = scenario_count;
    return (scenario_count > 0U) ? 1 : 0;
}

int visualizer_load_scenarios_from_files(int argc, char **argv, ScenarioSet *scenario_set)
{
    int argi;

    if ((argc < 2) || (argv == NULL) || (scenario_set == NULL))
    {
        return 0;
    }

    scenario_set->count = 0U;
    scenario_set->active_index = 0U;

    for (argi = 1; argi < argc; ++argi)
    {
        char *json_buffer = NULL;
        unsigned int parsed_count = 0U;
        unsigned int remaining_capacity;

        if (argv[argi] == NULL)
        {
            return 0;
        }

        remaining_capacity = MAX_SCENARIOS - scenario_set->count;
        if (remaining_capacity == 0U)
        {
            return 0;
        }

        if (load_file_text(argv[argi], &json_buffer) == 0)
        {
            return 0;
        }

        if (parse_scenarios_json(json_buffer,
                                 &scenario_set->scenarios[scenario_set->count],
                                 remaining_capacity,
                                 &parsed_count) == 0)
        {
            free(json_buffer);
            return 0;
        }

        free(json_buffer);
        scenario_set->count += parsed_count;
    }

    return (scenario_set->count > 0U) ? 1 : 0;
}
