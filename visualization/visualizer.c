#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define MAX_FILE_SIZE (1024U * 1024U)
#define MAX_TICKS 512U
#define MAX_SCENARIO_NAME 64U
#define MAX_MODE_NAME 16U
#define MAX_SCENARIOS 32U

#define EXPECTED_SCHEMA_VERSION "1.0"

#define MAX_RPM 10000.0f
#define MIN_TEMP -50.0f
#define MAX_TEMP 200.0f
#define MIN_OIL 0.0f
#define MAX_OIL 10.0f
#define MIN_CONTROL 0.0f
#define MAX_CONTROL 100.0f

#define TEMP_SHUTDOWN_THRESHOLD 95.0f
#define TEMP_WARNING_THRESHOLD 85.0f
#define OIL_SHUTDOWN_THRESHOLD 2.5f
#define RPM_WARNING_THRESHOLD 3500.0f

#define DEFAULT_TICKS_PER_SECOND 6.0f
#define SHORT_SCENARIO_TICKS_PER_SECOND 2.5f
#define FONT_PATH "visualization/PxPlus_IBM_EGA_8x14.ttf"

/* ── UI COLOURS ─────────────────────────────────────────────────────────── */
/* Background layers */
#define COL_WINDOW_BG ((Color){9, 11, 18, 255})
#define COL_GRAD_TOP ((Color){11, 14, 22, 255})
#define COL_GRAD_BOT ((Color){7, 9, 16, 255})
#define COL_PANEL_BG ((Color){13, 16, 26, 255})
#define COL_NAV_BG ((Color){10, 12, 20, 255})
#define COL_TIMELINE_BG ((Color){12, 15, 24, 255})
#define COL_BAR_BG ((Color){18, 22, 34, 255})
#define COL_BADGE_BG ((Color){20, 25, 40, 255})
#define COL_SLIDER_TRACK_BG ((Color){22, 27, 44, 255})
#define COL_KNOB_SHADOW ((Color){8, 10, 18, 180})
/* Borders / dividers */
#define COL_HDR_BORDER ((Color){30, 36, 58, 255})
#define COL_PANEL_BORDER ((Color){30, 36, 56, 255})
#define COL_TIMELINE_BORDER ((Color){32, 38, 58, 255})
#define COL_SECTION_DIV ((Color){24, 29, 48, 255})
#define COL_TIMELINE_SUBDIV ((Color){26, 31, 50, 255})
#define COL_FAULT_BAR_BOR ((Color){38, 44, 64, 255})
#define COL_BAR_BORDER ((Color){40, 46, 66, 255})
#define COL_SLIDER_TRACK_BOR ((Color){44, 52, 76, 255})
#define COL_BADGE_BORDER ((Color){44, 54, 84, 255})
/* Grid */
#define COL_GRID_LINE ((Color){44, 49, 63, 180})
#define COL_GRID_BRIGHT ((Color){96, 104, 122, 220})
#define COL_GRID_VERT ((Color){38, 43, 58, 170})
/* Text - dark to light */
#define COL_CAPTION ((Color){68, 82, 118, 255})
#define COL_TIMELINE_TITLE ((Color){72, 85, 120, 255})
#define COL_SPEED_RUNNING ((Color){72, 86, 120, 255})
#define COL_SCEN_COUNTER ((Color){78, 94, 136, 255})
#define COL_SUBLABEL ((Color){90, 106, 148, 255})
#define COL_KEY ((Color){90, 170, 255, 255})
#define COL_KEY_DESC ((Color){110, 124, 155, 255})
#define COL_METER_LABEL ((Color){120, 134, 165, 255})
#define COL_TICK_COUNTER ((Color){140, 155, 186, 255})
#define COL_BADGE_TEXT ((Color){148, 165, 205, 255})
#define COL_AXIS_LABEL ((Color){160, 168, 184, 255})
#define COL_FAULT_PCT_TEXT ((Color){170, 182, 212, 255})
#define COL_KNOB_FILL ((Color){220, 228, 245, 255})
#define COL_MODE_DEFAULT ((Color){180, 186, 200, 255})
#define COL_BRAK ((Color){50, 62, 94, 255})
/* Semantic status */
#define COL_OK ((Color){40, 167, 69, 255})
#define COL_WARNING ((Color){255, 193, 7, 255})
#define COL_SHUTDOWN ((Color){220, 53, 69, 255})
#define COL_WARNING_FILL ((Color){255, 193, 7, 200})
#define COL_SHUTDOWN_FILL ((Color){220, 53, 69, 210})
#define COL_WARNING_DASH ((Color){255, 193, 7, 160})
#define COL_SHUTDOWN_DASH ((Color){220, 53, 69, 170})
#define COL_OIL_SHUT_DASH ((Color){220, 53, 69, 120})
#define COL_WARNING_TINT ((Color){255, 193, 7, 26})
#define COL_SHUTDOWN_TINT ((Color){220, 53, 69, 34})
#define COL_SPEED_PAUSED ((Color){255, 193, 7, 220})
#define COL_END_NOTICE ((Color){255, 220, 100, 255})
#define COL_PLAYHEAD ((Color){255, 255, 255, 210})
/* Signal / sensor lines */
#define COL_RPM ((Color){52, 152, 219, 255})
#define COL_TEMP ((Color){255, 99, 132, 255})
#define COL_OIL ((Color){46, 204, 113, 255})
#define COL_CTRL ((Color){176, 132, 255, 255})
/* Slider accent */
#define COL_SLIDER_FILL ((Color){48, 128, 220, 255})
#define COL_SLIDER_RING ((Color){48, 128, 220, 200})

/* ── FONT SIZES (base, multiply by scale before use) ─────────────────────── */
#define FS_TINY 11.0f      /* small captions, fault rate labels */
#define FS_SMALL 12.0f     /* section header, key-hint desc text */
#define FS_KEY_HINT 13.0f  /* key-hint labels, meter labels */
#define FS_VALUE 16.0f     /* meter numeric values, log values */
#define FS_BADGE 15.0f     /* tick counter badge */
#define FS_MODE 22.0f      /* large engine-mode display */
#define FS_SCEN_NAME 20.0f /* scenario name in header */
/* Timeline font sizes are unscaled (drawn in absolute pixel coords) */
#define FS_TL_TITLE 18.0f
#define FS_TL_LEGEND 17.0f
#define FS_TL_AXIS 14.0f

/* ── LAYOUT (unscaled, multiply by scale where needed) ───────────────────── */
#define LAYOUT_PAD 16.0f
#define LAYOUT_HDR_H 50.0f
#define LAYOUT_NAV_H 30.0f
#define LAYOUT_MAIN_H 252.0f
#define LAYOUT_STATUS_W 310.0f
#define LAYOUT_COL_GAP 12.0f
#define LAYOUT_BAR_H 22.0f
#define LAYOUT_ROW_STEP 42.0f
#define LAYOUT_BADGE_H 28.0f
#define LAYOUT_KNOB_R 9.0f
#define LAYOUT_KNOB_SHADOW 11.0f
#define LAYOUT_TRACK_H 6.0f
#define LAYOUT_DOT_R 5.0f
#define LAYOUT_LEGEND_DOT_GAP 6.0f
#define LAYOUT_LEGEND_COL_GAP 18.0f
#define LAYOUT_SLIDER_INSET 30.0f /* space taken by slider inside timeline area */
#define LAYOUT_SLIDER_H 24.0f     /* slider control height */

/* Only used for the end-of-scenario replay hint (distinct from other label colours) */
#define COL_REPLAY_HINT ((Color){160, 174, 210, 255})

typedef enum
{
    LEVEL_OK = 0,
    LEVEL_WARNING = 1,
    LEVEL_SHUTDOWN = 2
} SeverityLevel;

typedef struct
{
    unsigned int tick;
    float rpm;
    float temp;
    float oil;
    int run;
    char result[12];
    float control;
    char engine_mode[MAX_MODE_NAME];
} TickData;

typedef struct
{
    char scenario[MAX_SCENARIO_NAME];
    TickData ticks[MAX_TICKS];
    unsigned int tick_count;
} ScenarioData;

typedef struct
{
    ScenarioData scenarios[MAX_SCENARIOS];
    unsigned int count;
    unsigned int active_index;
} ScenarioSet;

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

    if (strcmp(schema_version, EXPECTED_SCHEMA_VERSION) != 0)
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

        if (find_key(scenario_object_start, "\"requirement_id\"") == NULL)
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

static int load_scenarios_from_files(int argc, char **argv, ScenarioSet *scenario_set)
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

static SeverityLevel mode_to_level(const char *mode, const char *result)
{
    if ((mode != NULL) && (strcmp(mode, "SHUTDOWN") == 0))
    {
        return LEVEL_SHUTDOWN;
    }
    if ((result != NULL) && (strcmp(result, "SHUTDOWN") == 0))
    {
        return LEVEL_SHUTDOWN;
    }
    if ((mode != NULL) && (strcmp(mode, "WARNING") == 0))
    {
        return LEVEL_WARNING;
    }
    if ((result != NULL) && (strcmp(result, "WARNING") == 0))
    {
        return LEVEL_WARNING;
    }
    return LEVEL_OK;
}

static SeverityLevel temp_to_level(float temp)
{
    if (temp > TEMP_SHUTDOWN_THRESHOLD)
    {
        return LEVEL_SHUTDOWN;
    }
    if (temp >= TEMP_WARNING_THRESHOLD)
    {
        return LEVEL_WARNING;
    }
    return LEVEL_OK;
}

static SeverityLevel oil_to_level(float oil)
{
    if (oil < OIL_SHUTDOWN_THRESHOLD)
    {
        return LEVEL_SHUTDOWN;
    }
    return LEVEL_OK;
}

static SeverityLevel rpm_to_level(float rpm, float temp)
{
    if ((rpm >= RPM_WARNING_THRESHOLD) && (temp >= TEMP_WARNING_THRESHOLD))
    {
        return LEVEL_WARNING;
    }
    return LEVEL_OK;
}

static Color level_color(SeverityLevel level)
{
    if (level == LEVEL_SHUTDOWN)
    {
        return COL_SHUTDOWN;
    }
    if (level == LEVEL_WARNING)
    {
        return COL_WARNING;
    }
    return COL_OK;
}

static float clamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

static float lerp_float(float start_value, float end_value, float t)
{
    return start_value + ((end_value - start_value) * clamp01(t));
}

static void interpolate_tick(const ScenarioData *scenario, float playhead, TickData *out_tick)
{
    unsigned int base_index;
    unsigned int next_index;
    float phase;

    if ((scenario == NULL) || (out_tick == NULL) || (scenario->tick_count == 0U))
    {
        return;
    }

    if (playhead <= 0.0f)
    {
        *out_tick = scenario->ticks[0U];
        return;
    }

    if (playhead >= (float)(scenario->tick_count - 1U))
    {
        *out_tick = scenario->ticks[scenario->tick_count - 1U];
        return;
    }

    base_index = (unsigned int)playhead;
    next_index = base_index + 1U;
    phase = playhead - (float)base_index;

    *out_tick = scenario->ticks[base_index];
    out_tick->rpm = lerp_float(scenario->ticks[base_index].rpm, scenario->ticks[next_index].rpm, phase);
    out_tick->temp = lerp_float(scenario->ticks[base_index].temp, scenario->ticks[next_index].temp, phase);
    out_tick->oil = lerp_float(scenario->ticks[base_index].oil, scenario->ticks[next_index].oil, phase);
    out_tick->control = lerp_float(scenario->ticks[base_index].control, scenario->ticks[next_index].control, phase);

    if (phase >= 0.5f)
    {
        out_tick->tick = scenario->ticks[next_index].tick;
        out_tick->run = scenario->ticks[next_index].run;
        (void)strncpy(out_tick->result, scenario->ticks[next_index].result, sizeof(out_tick->result) - 1U);
        out_tick->result[sizeof(out_tick->result) - 1U] = '\0';
        (void)strncpy(out_tick->engine_mode,
                      scenario->ticks[next_index].engine_mode,
                      sizeof(out_tick->engine_mode) - 1U);
        out_tick->engine_mode[sizeof(out_tick->engine_mode) - 1U] = '\0';
    }
}

static Color mode_color(const char *engine_mode)
{
    if (engine_mode == NULL)
    {
        return COL_MODE_DEFAULT;
    }
    if ((strcmp(engine_mode, "INIT") == 0) || (strcmp(engine_mode, "STARTING") == 0))
    {
        return COL_RPM; /* blue: starting state shares the RPM signal colour */
    }
    if (strcmp(engine_mode, "WARNING") == 0)
    {
        return COL_WARNING;
    }
    if (strcmp(engine_mode, "SHUTDOWN") == 0)
    {
        return COL_SHUTDOWN;
    }
    return COL_OK;
}

static Color lerp_color(Color a, Color b, float t)
{
    Color out;
    float tt = clamp01(t);

    out.r = (unsigned char)roundf(lerp_float((float)a.r, (float)b.r, tt));
    out.g = (unsigned char)roundf(lerp_float((float)a.g, (float)b.g, tt));
    out.b = (unsigned char)roundf(lerp_float((float)a.b, (float)b.b, tt));
    out.a = (unsigned char)roundf(lerp_float((float)a.a, (float)b.a, tt));
    return out;
}

static void draw_text_font(const Font *font, const char *text, float x, float y, float size, Color color)
{
    if ((font == NULL) || (text == NULL))
    {
        return;
    }
    DrawTextEx(*font, text, (Vector2){x, y}, size, 1.0f, color);
}

static void draw_quit_modal(const Font *font, int selection, int screen_w, int screen_h, float scale)
{
    float box_w = 420.0f * scale;
    float box_h = 152.0f * scale;
    float box_x = ((float)screen_w - box_w) * 0.5f;
    float box_y = ((float)screen_h - box_h) * 0.5f;
    float btn_w = 80.0f * scale;
    float btn_h = 28.0f * scale;
    float btn_y = box_y + 66.0f * scale;
    float yes_x = box_x + box_w * 0.5f - btn_w - 10.0f * scale;
    float no_x = box_x + box_w * 0.5f + 10.0f * scale;
    float title_fs = FS_MODE * scale;
    float opt_fs = FS_VALUE * scale;
    float hint_fs = FS_TINY * scale;
    const char *title = "QUIT?";
    const char *hint = "[LT RT]  Select   [ENTER]  Confirm   [ESC]  Cancel";
    Vector2 sz;
    Color yes_bg, yes_bor, yes_txt;
    Color no_bg, no_bor, no_txt;

    /* full-screen dim overlay */
    DrawRectangle(0, 0, screen_w, screen_h, (Color){0, 0, 0, 160});

    /* panel shadow */
    DrawRectangle((int)(box_x + 4.0f), (int)(box_y + 4.0f), (int)box_w, (int)box_h, (Color){0, 0, 0, 110});
    /* panel body */
    DrawRectangle((int)box_x, (int)box_y, (int)box_w, (int)box_h, COL_PANEL_BG);
    DrawRectangleLines((int)box_x, (int)box_y, (int)box_w, (int)box_h, COL_PANEL_BORDER);

    /* title */
    sz = MeasureTextEx(*font, title, title_fs, 1.0f);
    draw_text_font(font, title,
                   box_x + (box_w - sz.x) * 0.5f,
                   box_y + 14.0f * scale,
                   title_fs, RAYWHITE);

    /* divider */
    DrawLine((int)(box_x + 16.0f * scale), (int)(box_y + 54.0f * scale),
             (int)(box_x + box_w - 16.0f * scale), (int)(box_y + 54.0f * scale),
             COL_SECTION_DIV);

    /* YES button */
    yes_bg = COL_BADGE_BG;
    yes_bor = (selection == 1) ? RAYWHITE : COL_BADGE_BORDER;
    yes_txt = RAYWHITE;
    DrawRectangle((int)yes_x, (int)btn_y, (int)btn_w, (int)btn_h, yes_bg);
    DrawRectangleLines((int)yes_x, (int)btn_y, (int)btn_w, (int)btn_h, yes_bor);
    sz = MeasureTextEx(*font, "YES", opt_fs, 1.0f);
    draw_text_font(font, "YES",
                   yes_x + (btn_w - sz.x) * 0.5f,
                   btn_y + (btn_h - sz.y) * 0.5f,
                   opt_fs, yes_txt);

    /* NO button */
    no_bg = COL_BADGE_BG;
    no_bor = (selection == 0) ? RAYWHITE : COL_BADGE_BORDER;
    no_txt = RAYWHITE;
    DrawRectangle((int)no_x, (int)btn_y, (int)btn_w, (int)btn_h, no_bg);
    DrawRectangleLines((int)no_x, (int)btn_y, (int)btn_w, (int)btn_h, no_bor);
    sz = MeasureTextEx(*font, "NO", opt_fs, 1.0f);
    draw_text_font(font, "NO",
                   no_x + (btn_w - sz.x) * 0.5f,
                   btn_y + (btn_h - sz.y) * 0.5f,
                   opt_fs, no_txt);

    /* navigation hint */
    sz = MeasureTextEx(*font, hint, hint_fs, 1.0f);
    draw_text_font(font, hint,
                   box_x + (box_w - sz.x) * 0.5f,
                   box_y + box_h - 20.0f * scale,
                   hint_fs, COL_SUBLABEL);
}

static float draw_key_hint(const Font *font,
                           const char *key,
                           const char *desc,
                           float x,
                           float y,
                           float font_size)
{
    Color key_color = COL_KEY;
    Color brak_color = COL_BRAK;
    Color desc_color = COL_KEY_DESC;
    float cx = x;
    float w;

    w = MeasureTextEx(*font, "[", font_size, 1.0f).x;
    draw_text_font(font, "[", cx, y, font_size, brak_color);
    cx += w;
    w = MeasureTextEx(*font, key, font_size, 1.0f).x;
    draw_text_font(font, key, cx, y, font_size, key_color);
    cx += w;
    w = MeasureTextEx(*font, "]", font_size, 1.0f).x;
    draw_text_font(font, "]", cx, y, font_size, brak_color);
    cx += w + 4.0f;
    w = MeasureTextEx(*font, desc, font_size, 1.0f).x;
    draw_text_font(font, desc, cx, y, font_size, desc_color);
    cx += w;
    return cx - x;
}

static void draw_meter(const Font *font,
                       const char *label,
                       float value,
                       float max_value,
                       Rectangle bar_area,
                       Rectangle value_area,
                       SeverityLevel level,
                       const char *unit,
                       float warn_threshold,
                       float shutdown_threshold,
                       float scale)
{
    float ratio;
    int fill_width;
    char value_text[64];
    float lbl_fs = FS_KEY_HINT * scale;
    float val_fs = FS_VALUE * scale;
    Vector2 value_size;
    float value_x;
    float value_y;

    ratio = (max_value > 0.0f) ? (value / max_value) : 0.0f;
    ratio = clamp01(ratio);
    fill_width = (int)(ratio * bar_area.width);

    draw_text_font(font, label, bar_area.x, bar_area.y - lbl_fs - 3.0f * scale, lbl_fs, COL_METER_LABEL);
    DrawRectangleRec(bar_area, COL_BAR_BG);
    if (fill_width > 0)
    {
        DrawRectangle((int)bar_area.x, (int)bar_area.y, fill_width, (int)bar_area.height, level_color(level));
    }
    DrawRectangleLines((int)bar_area.x, (int)bar_area.y, (int)bar_area.width, (int)bar_area.height, COL_BAR_BORDER);

    if ((warn_threshold > 0.0f) && (warn_threshold < max_value))
    {
        int warn_x = (int)(bar_area.x + (warn_threshold / max_value) * bar_area.width);
        DrawLine(warn_x, (int)(bar_area.y + 2), warn_x, (int)(bar_area.y + bar_area.height - 2), COL_WARNING_FILL);
    }
    if ((shutdown_threshold > 0.0f) && (shutdown_threshold < max_value))
    {
        int shut_x = (int)(bar_area.x + (shutdown_threshold / max_value) * bar_area.width);
        DrawLine(shut_x, (int)(bar_area.y + 2), shut_x, (int)(bar_area.y + bar_area.height - 2), COL_SHUTDOWN_FILL);
    }

    (void)snprintf(value_text, sizeof(value_text), "%.2f %s", value, unit);
    value_size = MeasureTextEx(*font, value_text, val_fs, 1.0f);
    value_x = value_area.x + value_area.width - value_size.x;
    if (value_x < value_area.x)
    {
        value_x = value_area.x;
    }
    value_y = bar_area.y + (bar_area.height - val_fs) * 0.5f;
    draw_text_font(font, value_text, value_x, value_y, val_fs, RAYWHITE);
}

static void draw_dashed_hline(int x0, int x1, int y, int dash, int gap, Color color)
{
    int x = x0;
    while (x < x1)
    {
        int end = x + dash;
        if (end > x1)
        {
            end = x1;
        }
        DrawLine(x, y, end, y, color);
        x = end + gap;
    }
}

/* Draw a standard info panel: filled background, border, header caption, and a
   horizontal divider line below the header row.  Returns the Y coordinate of
   the content area (just below the divider). */
static float draw_panel_header(const Font *font, Rectangle area, const char *title, float scale)
{
    float title_y = area.y + 10.0f * scale;
    float divider_y = area.y + 31.0f * scale;

    DrawRectangleRec(area, COL_PANEL_BG);
    DrawRectangleLines((int)area.x, (int)area.y, (int)area.width, (int)area.height, COL_PANEL_BORDER);
    draw_text_font(font, title, area.x + 14.0f * scale, title_y, FS_SMALL * scale, COL_CAPTION);
    DrawLine((int)(area.x + 1), (int)divider_y,
             (int)(area.x + area.width - 1), (int)divider_y,
             COL_SECTION_DIV);

    return divider_y;
}

/* Draw a single fault-rate row: label, filled progress bar, and percentage
   text.  bar_x/bar_w define the bar rect; label is drawn at label_x on row_y. */
static void draw_fault_rate_row(const Font *font,
                                const char *label,
                                float value_pct,
                                float label_x,
                                float bar_x,
                                float bar_w,
                                float bar_h,
                                float row_y,
                                Color fill_color,
                                float scale)
{
    char pct_text[20];
    float text_x = bar_x + bar_w + 6.0f * scale;

    draw_text_font(font, label, label_x, row_y, FS_TINY * scale, COL_SUBLABEL);

    DrawRectangle((int)bar_x, (int)row_y, (int)bar_w, (int)bar_h, COL_BAR_BG);
    DrawRectangle((int)bar_x, (int)row_y, (int)(bar_w * (value_pct / 100.0f)), (int)bar_h, fill_color);
    DrawRectangleLines((int)bar_x, (int)row_y, (int)bar_w, (int)bar_h, COL_FAULT_BAR_BOR);

    (void)snprintf(pct_text, sizeof(pct_text), "%.1f%%", value_pct);
    draw_text_font(font, pct_text, text_x, row_y, FS_TINY * scale, COL_FAULT_PCT_TEXT);
}

static void draw_timeline(const Font *font, const ScenarioData *scenario, float playhead, Rectangle area, float scale)
{
    int i;
    int plot_left;
    int plot_right;
    int plot_top;
    int plot_bottom;
    int plot_w;
    int plot_h;

    if ((scenario == NULL) || (scenario->tick_count == 0U))
    {
        return;
    }

    DrawRectangleRec(area, COL_TIMELINE_BG);
    DrawRectangleLines((int)area.x, (int)area.y, (int)area.width, (int)area.height, COL_TIMELINE_BORDER);

    draw_text_font(font, "TIMELINE", area.x + 14.0f, area.y + 8.0f, FS_TL_TITLE, COL_TIMELINE_TITLE);
    DrawLine((int)(area.x + 1), (int)(area.y + 32.0f),
             (int)(area.x + area.width - 1), (int)(area.y + 32.0f),
             COL_TIMELINE_SUBDIV);

    {
        unsigned int active_tick = scenario->ticks[(unsigned int)(playhead + 0.5f)].tick;
        char tick_label[48];
        Vector2 tl_sz;
        float tl_x;
        (void)snprintf(tick_label, sizeof(tick_label), "Tick %u / %u", active_tick,
                       scenario->ticks[scenario->tick_count - 1U].tick);
        tl_sz = MeasureTextEx(*font, tick_label, FS_TL_TITLE, 1.0f);
        tl_x = area.x + area.width * 0.5f - tl_sz.x * 0.5f;
        draw_text_font(font, tick_label, tl_x, area.y + 8.0f, FS_TL_TITLE, COL_TICK_COUNTER);
    }

    plot_left = (int)area.x + 52;
    plot_right = (int)(area.x + area.width) - 24;
    plot_top = (int)area.y + 48;
    plot_bottom = (int)(area.y + area.height - 30.0f * scale - 30.0f);
    plot_w = plot_right - plot_left;
    plot_h = plot_bottom - plot_top;

    for (i = 0; i <= 8; ++i)
    {
        int gy = plot_top + (i * plot_h) / 8;
        Color grid_color = (i == 8) ? COL_GRID_BRIGHT : COL_GRID_LINE;
        DrawLine(plot_left, gy, plot_right, gy, grid_color);
        if ((i % 2) == 0)
        {
            char y_label[16];
            (void)snprintf(y_label, sizeof(y_label), "%d%%", 100 - ((i * 100) / 8));
            draw_text_font(font, y_label, area.x + 8.0f, (float)gy - 8.0f, FS_TL_AXIS, COL_AXIS_LABEL);
        }
    }

    if (scenario->tick_count > 1U)
    {
        unsigned int x_steps = (scenario->tick_count < 9U) ? (scenario->tick_count - 1U) : 8U;
        unsigned int s;
        for (s = 0U; s <= x_steps; ++s)
        {
            float t = (x_steps == 0U) ? 0.0f : ((float)s / (float)x_steps);
            int gx = plot_left + (int)(t * (float)plot_w);
            unsigned int tick_idx = (unsigned int)(t * (float)(scenario->tick_count - 1U));
            char x_label[16];
            DrawLine(gx, plot_top, gx, plot_bottom, COL_GRID_VERT);
            (void)snprintf(x_label, sizeof(x_label), "%u", scenario->ticks[tick_idx].tick);
            draw_text_font(font, x_label, (float)gx - 8.0f, (float)plot_bottom + 8.0f, FS_TL_AXIS, COL_AXIS_LABEL);
        }
    }

    {
        int temp_warn_y = plot_bottom - (int)((TEMP_WARNING_THRESHOLD / 120.0f) * (float)plot_h);
        int temp_shutdown_y = plot_bottom - (int)((TEMP_SHUTDOWN_THRESHOLD / 120.0f) * (float)plot_h);
        int oil_shutdown_y = plot_bottom - (int)((OIL_SHUTDOWN_THRESHOLD / 5.0f) * (float)plot_h);
        draw_dashed_hline(plot_left, plot_right, temp_warn_y, 10, 5, COL_WARNING_DASH);
        draw_dashed_hline(plot_left, plot_right, temp_shutdown_y, 10, 5, COL_SHUTDOWN_DASH);
        draw_dashed_hline(plot_left, plot_right, oil_shutdown_y, 10, 5, COL_OIL_SHUT_DASH);
    }

    if (scenario->tick_count > 1U)
    {
        unsigned int idx;
        for (idx = 1U; idx < scenario->tick_count; ++idx)
        {
            SeverityLevel level = mode_to_level(scenario->ticks[idx].engine_mode, scenario->ticks[idx].result);
            float t0 = (float)(idx - 1U) / (float)(scenario->tick_count - 1U);
            float t1 = (float)idx / (float)(scenario->tick_count - 1U);
            int sx = plot_left + (int)(t0 * (float)plot_w);
            int ex = plot_left + (int)(t1 * (float)plot_w);

            if (level == LEVEL_WARNING)
            {
                DrawRectangle(sx, plot_top, ex - sx, plot_h, COL_WARNING_TINT);
            }
            else if (level == LEVEL_SHUTDOWN)
            {
                DrawRectangle(sx, plot_top, ex - sx, plot_h, COL_SHUTDOWN_TINT);
            }
        }

        for (idx = 1U; idx < scenario->tick_count; ++idx)
        {
            float t0 = (float)(idx - 1U) / (float)(scenario->tick_count - 1U);
            float t1 = (float)idx / (float)(scenario->tick_count - 1U);
            int x0 = plot_left + (int)(t0 * (float)plot_w);
            int x1 = plot_left + (int)(t1 * (float)plot_w);

            float rpm0 = clamp01(scenario->ticks[idx - 1U].rpm / 5000.0f);
            float rpm1 = clamp01(scenario->ticks[idx].rpm / 5000.0f);
            float temp0 = clamp01(scenario->ticks[idx - 1U].temp / 120.0f);
            float temp1 = clamp01(scenario->ticks[idx].temp / 120.0f);
            float oil0 = clamp01(scenario->ticks[idx - 1U].oil / 5.0f);
            float oil1 = clamp01(scenario->ticks[idx].oil / 5.0f);
            float ctrl0 = clamp01(scenario->ticks[idx - 1U].control / 100.0f);
            float ctrl1 = clamp01(scenario->ticks[idx].control / 100.0f);

            int yr0 = plot_bottom - (int)(rpm0 * (float)plot_h);
            int yr1 = plot_bottom - (int)(rpm1 * (float)plot_h);
            int yt0 = plot_bottom - (int)(temp0 * (float)plot_h);
            int yt1 = plot_bottom - (int)(temp1 * (float)plot_h);
            int yo0 = plot_bottom - (int)(oil0 * (float)plot_h);
            int yo1 = plot_bottom - (int)(oil1 * (float)plot_h);
            int yc0 = plot_bottom - (int)(ctrl0 * (float)plot_h);
            int yc1 = plot_bottom - (int)(ctrl1 * (float)plot_h);

            DrawLineEx((Vector2){(float)x0, (float)yr0}, (Vector2){(float)x1, (float)yr1}, 2.0f, COL_RPM);
            DrawLineEx((Vector2){(float)x0, (float)yt0}, (Vector2){(float)x1, (float)yt1}, 2.0f, COL_TEMP);
            DrawLineEx((Vector2){(float)x0, (float)yo0}, (Vector2){(float)x1, (float)yo1}, 2.0f, COL_OIL);
            DrawLineEx((Vector2){(float)x0, (float)yc0}, (Vector2){(float)x1, (float)yc1}, 2.0f, COL_CTRL);
        }

        {
            float marker_t = clamp01(playhead / (float)(scenario->tick_count - 1U));
            int marker_x = plot_left + (int)(marker_t * (float)plot_w);
            DrawLineEx((Vector2){(float)marker_x, (float)plot_top},
                       (Vector2){(float)marker_x, (float)plot_bottom},
                       2.0f, COL_PLAYHEAD);
        }
    }

    /* Compact right-to-left legend with colour dots */
    {
        float ley = area.y + 8.0f;
        float lfs2 = FS_TL_LEGEND;
        float dot_r = LAYOUT_DOT_R;
        float lx = area.x + area.width - 12.0f;
        Vector2 lsz;

        /* Entries drawn right-to-left: CTRL → OIL → TEMP → RPM */
        static const struct
        {
            const char *name;
            Color col;
        } k_legend[] = {
            {"CTRL", COL_CTRL},
            {"OIL", COL_OIL},
            {"TEMP", COL_TEMP},
            {"RPM", COL_RPM},
        };
        int li;
        for (li = 0; li < 4; ++li)
        {
            lsz = MeasureTextEx(*font, k_legend[li].name, lfs2, 1.0f);
            lx -= lsz.x;
            draw_text_font(font, k_legend[li].name, lx, ley, lfs2, k_legend[li].col);
            lx -= dot_r * 2.0f + LAYOUT_LEGEND_DOT_GAP;
            DrawCircle((int)(lx + dot_r), (int)(ley + lfs2 * 0.5f), dot_r, k_legend[li].col);
            lx -= LAYOUT_LEGEND_COL_GAP;
        }
    }
}

static void draw_slider(const Font *font,
                        Rectangle slider,
                        const ScenarioData *scenario,
                        float playhead,
                        float restart_feedback_timer)
{
    float knob_t;
    int knob_x;

    if ((scenario == NULL) || (scenario->tick_count == 0U))
    {
        return;
    }

    (void)font;

    /* Track + knob */
    {
        int cy = (int)(slider.y + slider.height * 0.5f);
        int th = 6;
        int tx = (int)slider.x;
        int tw = (int)slider.width;
        int rx = 3;

        knob_t = (scenario->tick_count > 1U) ? clamp01(playhead / (float)(scenario->tick_count - 1U)) : 0.0f;
        knob_x = (int)(slider.x + (slider.width * knob_t));

        /* Background track */
        DrawRectangle(tx, cy - th / 2, tw, th, COL_SLIDER_TRACK_BG);
        DrawRectangleLines(tx, cy - th / 2, tw, th, COL_SLIDER_TRACK_BOR);
        /* Filled progress */
        if (knob_t > 0.0f)
        {
            int fw = (int)(tw * knob_t);
            DrawRectangle(tx + rx, cy - th / 2, fw - rx, th, COL_SLIDER_FILL);
        }

        /* Knob: shadow ring, white fill, blue accent */
        DrawCircle(knob_x, cy, LAYOUT_KNOB_SHADOW, COL_KNOB_SHADOW);
        DrawCircle(knob_x, cy, LAYOUT_KNOB_R, COL_KNOB_FILL);
        DrawCircleLines(knob_x, cy, LAYOUT_KNOB_SHADOW, COL_SLIDER_RING);
    }

    (void)restart_feedback_timer;
}

static void compute_cumulative_metrics(const ScenarioData *scenario, float playhead, float *warning_pct, float *shutdown_pct)
{
    unsigned int i;
    unsigned int warn_count = 0U;
    unsigned int shut_count = 0U;
    unsigned int active_count;

    if ((scenario == NULL) || (warning_pct == NULL) || (shutdown_pct == NULL) || (scenario->tick_count == 0U))
    {
        return;
    }

    active_count = (unsigned int)(playhead + 0.5f) + 1U;
    if (active_count > scenario->tick_count)
    {
        active_count = scenario->tick_count;
    }

    for (i = 0U; i < active_count; ++i)
    {
        SeverityLevel level = mode_to_level(scenario->ticks[i].engine_mode, scenario->ticks[i].result);
        if (level == LEVEL_WARNING)
        {
            warn_count++;
        }
        else if (level == LEVEL_SHUTDOWN)
        {
            shut_count++;
        }
    }

    *warning_pct = (100.0f * (float)warn_count) / (float)active_count;
    *shutdown_pct = (100.0f * (float)shut_count) / (float)active_count;
}

static float screen_scale(int width, int height)
{
    float sx = (float)width / (float)WINDOW_WIDTH;
    float sy = (float)height / (float)WINDOW_HEIGHT;
    return (sx < sy) ? sx : sy;
}

static void run_visualizer(ScenarioSet *scenario_set)
{
    float playhead = 0.0f;
    float ticks_per_second;
    float restart_feedback_timer = 0.0f;
    int paused = 0;
    int dragging_slider = 0;
    int quit_modal_open = 0;
    int quit_modal_selection = 0; /* 0 = No (default), 1 = Yes */
    int should_quit = 0;
    Color animated_mode_color;
    Font ui_font;
    int custom_font_loaded = 0;

    if ((scenario_set == NULL) || (scenario_set->count == 0U))
    {
        return;
    }

    ticks_per_second = (scenario_set->scenarios[scenario_set->active_index].tick_count <= 8U)
                           ? SHORT_SCENARIO_TICKS_PER_SECOND
                           : DEFAULT_TICKS_PER_SECOND;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Engine Control Scenario Visualizer");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    ui_font = LoadFontEx(FONT_PATH, 28, NULL, 0);
    if ((ui_font.texture.id > 0U) && (ui_font.glyphCount > 0))
    {
        SetTextureFilter(ui_font.texture, TEXTURE_FILTER_POINT);
        custom_font_loaded = 1;
    }
    else
    {
        ui_font = GetFontDefault();
    }

    animated_mode_color = mode_color(scenario_set->scenarios[scenario_set->active_index].ticks[0U].engine_mode);

    while (!WindowShouldClose() && (should_quit == 0))
    {
        ScenarioData *scenario = &scenario_set->scenarios[scenario_set->active_index];
        TickData interpolated_tick;
        const TickData *tick;
        int screen_w = GetScreenWidth();
        int screen_h = GetScreenHeight();
        float scale = screen_scale(screen_w, screen_h);
        float pad = LAYOUT_PAD * scale;
        float hdr_h = LAYOUT_HDR_H * scale;
        float nav_h = LAYOUT_NAV_H * scale;
        float content_y = hdr_h + nav_h + 8.0f * scale;
        float main_h = LAYOUT_MAIN_H * scale;
        float status_panel_w = LAYOUT_STATUS_W * scale;
        float col_gap = LAYOUT_COL_GAP * scale;
        float gauges_x = pad;
        float gauges_w = (float)screen_w - (pad * 2.0f) - status_panel_w - col_gap;
        float status_x = gauges_x + gauges_w + col_gap;
        float gc_x = gauges_x + 14.0f * scale;
        float gc_w = gauges_w - 28.0f * scale;
        float bar_col_w = gc_w * 0.68f;
        float val_col_w = gc_w - bar_col_w - 10.0f * scale;
        float val_col_x = gc_x + bar_col_w + 10.0f * scale;
        float bar_h = LAYOUT_BAR_H * scale;
        float row_step = LAYOUT_ROW_STEP * scale;
        float m_row0_y = content_y + 44.0f * scale;
        float m_row1_y = m_row0_y + row_step;
        float m_row2_y = m_row1_y + row_step;
        Rectangle gauges_panel = {gauges_x, content_y, gauges_w, main_h};
        Rectangle metrics_area = {status_x, content_y, status_panel_w, main_h};
        Rectangle rpm_bar = {gc_x, m_row0_y, bar_col_w, bar_h};
        Rectangle rpm_val = {val_col_x, m_row0_y, val_col_w, bar_h};
        Rectangle temp_bar = {gc_x, m_row1_y, bar_col_w, bar_h};
        Rectangle temp_val = {val_col_x, m_row1_y, val_col_w, bar_h};
        Rectangle oil_bar = {gc_x, m_row2_y, bar_col_w, bar_h};
        Rectangle oil_val = {val_col_x, m_row2_y, val_col_w, bar_h};
        float timeline_y = content_y + main_h + 10.0f * scale;
        float timeline_h_val = (float)screen_h - timeline_y - pad;
        Rectangle timeline = {pad, timeline_y, (float)screen_w - 2.0f * pad, timeline_h_val};
        Rectangle slider = {timeline.x + 52.0f,
                            timeline.y + timeline.height - LAYOUT_SLIDER_INSET * scale,
                            timeline.width - 76.0f,
                            LAYOUT_SLIDER_H * scale};
        float warning_pct = 0.0f;
        float shutdown_pct = 0.0f;

        compute_cumulative_metrics(scenario, playhead, &warning_pct, &shutdown_pct);

        if (IsKeyPressed(KEY_F11))
        {
            if (!IsWindowFullscreen())
            {
                int monitor = GetCurrentMonitor();
                SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
                ToggleFullscreen();
            }
            else
            {
                ToggleFullscreen();
                SetWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
            }
        }
        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (quit_modal_open != 0)
            {
                quit_modal_open = 0;
                quit_modal_selection = 0;
            }
            else
            {
                quit_modal_open = 1;
                quit_modal_selection = 0;
            }
        }
        if (quit_modal_open != 0)
        {
            if (IsKeyPressed(KEY_LEFT))
            {
                quit_modal_selection = 1; /* move to YES */
            }
            if (IsKeyPressed(KEY_RIGHT))
            {
                quit_modal_selection = 0; /* move to NO */
            }
            if (IsKeyPressed(KEY_ENTER))
            {
                if (quit_modal_selection == 1)
                {
                    should_quit = 1;
                }
                else
                {
                    quit_modal_open = 0;
                    quit_modal_selection = 0;
                }
            }
        }
        else
        {
            if (IsKeyPressed(KEY_SPACE))
            {
                paused = (paused == 0) ? 1 : 0;
            }
            if (IsKeyPressed(KEY_R))
            {
                playhead = 0.0f;
                paused = 0;
                restart_feedback_timer = 0.6f;
            }
            if (IsKeyPressed(KEY_RIGHT))
            {
                playhead += 1.0f;
                if (playhead > (float)(scenario->tick_count - 1U))
                {
                    playhead = (float)(scenario->tick_count - 1U);
                }
                paused = 1;
            }
            if (IsKeyPressed(KEY_LEFT))
            {
                playhead -= 1.0f;
                if (playhead < 0.0f)
                {
                    playhead = 0.0f;
                }
                paused = 1;
            }
            if (IsKeyPressed(KEY_UP))
            {
                ticks_per_second += 1.0f;
                if (ticks_per_second > 20.0f)
                {
                    ticks_per_second = 20.0f;
                }
            }
            if (IsKeyPressed(KEY_DOWN))
            {
                ticks_per_second -= 1.0f;
                if (ticks_per_second < 1.0f)
                {
                    ticks_per_second = 1.0f;
                }
            }
            if (IsKeyPressed(KEY_TAB) && (scenario_set->count > 1U))
            {
                scenario_set->active_index = (scenario_set->active_index + 1U) % scenario_set->count;
                scenario = &scenario_set->scenarios[scenario_set->active_index];
                playhead = 0.0f;
                paused = 0;
                restart_feedback_timer = 0.6f;
                ticks_per_second = (scenario->tick_count <= 8U) ? SHORT_SCENARIO_TICKS_PER_SECOND : DEFAULT_TICKS_PER_SECOND;
                animated_mode_color = mode_color(scenario->ticks[0U].engine_mode);
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), slider))
        {
            dragging_slider = 1;
            paused = 1;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            dragging_slider = 0;
        }
        if (dragging_slider != 0)
        {
            float rel = (GetMousePosition().x - slider.x) / slider.width;
            rel = clamp01(rel);
            playhead = rel * (float)(scenario->tick_count - 1U);
        }

        if ((paused == 0) && (dragging_slider == 0) && (playhead < (float)(scenario->tick_count - 1U)))
        {
            playhead += GetFrameTime() * ticks_per_second;
            if (playhead > (float)(scenario->tick_count - 1U))
            {
                playhead = (float)(scenario->tick_count - 1U);
            }
        }

        if (restart_feedback_timer > 0.0f)
        {
            restart_feedback_timer -= GetFrameTime();
            if (restart_feedback_timer < 0.0f)
            {
                restart_feedback_timer = 0.0f;
            }
        }

        interpolate_tick(scenario, playhead, &interpolated_tick);
        tick = &interpolated_tick;

        animated_mode_color = lerp_color(animated_mode_color, mode_color(tick->engine_mode), GetFrameTime() * 8.0f);

        BeginDrawing();
        ClearBackground(COL_WINDOW_BG);
        DrawRectangleGradientV(0, 0, screen_w, screen_h, COL_GRAD_TOP, COL_GRAD_BOT);

        /* ── HEADER BAR ────────────────────────────────── */
        DrawRectangle(0, 0, screen_w, (int)hdr_h, COL_PANEL_BG);
        DrawLine(0, (int)hdr_h - 1, screen_w, (int)hdr_h - 1, COL_HDR_BORDER);
        {
            char scen_label[48];
            char tick_str[48];
            Vector2 tick_sz;
            float badge_w;
            float badge_x;
            float badge_y;
            float badge_h = LAYOUT_BADGE_H * scale;

            (void)snprintf(scen_label, sizeof(scen_label),
                           "SCENARIO  %u / %u",
                           scenario_set->active_index + 1U, scenario_set->count);
            draw_text_font(&ui_font, scen_label, pad, 7.0f * scale, FS_TINY * scale, COL_SCEN_COUNTER);
            draw_text_font(&ui_font, scenario->scenario, pad, 21.0f * scale, FS_SCEN_NAME * scale, RAYWHITE);

            (void)snprintf(tick_str, sizeof(tick_str), "Tick  %u / %u",
                           tick->tick, scenario->tick_count);
            tick_sz = MeasureTextEx(ui_font, tick_str, FS_BADGE * scale, 1.0f);
            badge_w = tick_sz.x + 20.0f * scale;
            badge_x = (float)screen_w - badge_w - pad;
            badge_y = (hdr_h - badge_h) * 0.5f;
            DrawRectangle((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h, COL_BADGE_BG);
            DrawRectangleLines((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h, COL_BADGE_BORDER);
            draw_text_font(&ui_font, tick_str,
                           badge_x + 10.0f * scale,
                           badge_y + (badge_h - FS_BADGE * scale) * 0.5f,
                           FS_BADGE * scale, COL_BADGE_TEXT);
        }

        /* ── CONTROLS BAR ──────────────────────────────── */
        DrawRectangle(0, (int)hdr_h, screen_w, (int)nav_h, COL_NAV_BG);
        DrawLine(0, (int)(hdr_h + nav_h), screen_w, (int)(hdr_h + nav_h), COL_SECTION_DIV);
        {
            float kx = pad;
            float ky = hdr_h + (nav_h - FS_KEY_HINT * scale) * 0.5f;
            float kfs = FS_KEY_HINT * scale;
            float gap2 = 20.0f * scale;
            char speed_str[32];
            Vector2 sp_sz;

            kx += draw_key_hint(&ui_font, "SPC", "Pause", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "R", "Restart", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "UP DN", "Speed", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "LT RT", "Step", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "TAB", "Switch", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "F11", "Fullscreen", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "ESC", "Quit", kx, ky, kfs);
            (void)kx;

            (void)snprintf(speed_str, sizeof(speed_str), "%.0f tk/s%s",
                           ticks_per_second, (paused != 0) ? "  PAUSED" : "");
            sp_sz = MeasureTextEx(ui_font, speed_str, kfs, 1.0f);
            draw_text_font(&ui_font, speed_str,
                           (float)screen_w - sp_sz.x - pad, ky, kfs,
                           (paused != 0) ? COL_SPEED_PAUSED : COL_SPEED_RUNNING);
        }

        /* ── GAUGES PANEL ──────────────────────────────── */
        draw_panel_header(&ui_font, gauges_panel, "SENSOR READINGS", scale);

        draw_meter(&ui_font, "RPM", tick->rpm, 5000.0f,
                   rpm_bar, rpm_val,
                   rpm_to_level(tick->rpm, tick->temp),
                   "rpm", RPM_WARNING_THRESHOLD, 0.0f, scale);
        draw_meter(&ui_font, "Temperature", tick->temp, 120.0f,
                   temp_bar, temp_val,
                   temp_to_level(tick->temp),
                   "C", TEMP_WARNING_THRESHOLD, TEMP_SHUTDOWN_THRESHOLD, scale);
        draw_meter(&ui_font, "Oil Pressure", tick->oil, 5.0f,
                   oil_bar, oil_val,
                   oil_to_level(tick->oil),
                   "bar", 0.0f, OIL_SHUTDOWN_THRESHOLD, scale);
        /* ── STATUS PANEL ──────────────────────────────── */
        draw_panel_header(&ui_font, metrics_area, "ENGINE STATUS", scale);

        /* Mode (left) + Result/Run (right), same horizontal band */
        {
            float lx = metrics_area.x + 14.0f * scale;
            float rx = metrics_area.x + metrics_area.width * 0.54f;
            float cap_y = metrics_area.y + 38.0f * scale;
            float cap_fs = FS_TINY * scale;
            float val_fs = FS_KEY_HINT * scale;
            float mode_fs = FS_MODE * scale;
            char run_val[16];
            Color result_c;

            result_c = (strcmp(tick->result, "OK") == 0)
                           ? COL_OIL /* green  */
                       : (strcmp(tick->result, "WARNING") == 0)
                           ? COL_WARNING   /* amber  */
                           : COL_SHUTDOWN; /* red   */

            /* Left: MODE caption + big mode text */
            draw_text_font(&ui_font, "MODE", lx, cap_y, cap_fs, COL_CAPTION);
            draw_text_font(&ui_font, tick->engine_mode, lx, cap_y + cap_fs + 4.0f * scale,
                           mode_fs, animated_mode_color);

            /* Right: RESULT caption + value, then RUN caption + value */
            draw_text_font(&ui_font, "RESULT", rx, cap_y, cap_fs, COL_CAPTION);
            draw_text_font(&ui_font, tick->result, rx, cap_y + cap_fs + 4.0f * scale, val_fs, result_c);
            draw_text_font(&ui_font, "RUN", rx, cap_y + cap_fs + 4.0f * scale + val_fs + 10.0f * scale,
                           cap_fs, COL_CAPTION);
            (void)snprintf(run_val, sizeof(run_val), "%d", tick->run);
            draw_text_font(&ui_font, run_val,
                           rx, cap_y + cap_fs * 2.0f + val_fs + 8.0f * scale + 10.0f * scale,
                           val_fs, RAYWHITE);
        }

        /* Divider */
        DrawLine((int)(metrics_area.x + 14.0f * scale), (int)(metrics_area.y + 110.0f * scale),
                 (int)(metrics_area.x + metrics_area.width - 14.0f * scale),
                 (int)(metrics_area.y + 110.0f * scale),
                 COL_SECTION_DIV);

        /* Session fault rate */
        draw_text_font(&ui_font, "SESSION FAULT RATE",
                       metrics_area.x + 14.0f * scale, metrics_area.y + 118.0f * scale,
                       FS_TINY * scale, COL_CAPTION);
        {
            float bx2 = metrics_area.x + 14.0f * scale;
            float bw2 = metrics_area.width - 28.0f * scale;
            float bh2 = 9.0f * scale;
            float lbl_w2 = 68.0f * scale;
            float tr_x = bx2 + lbl_w2;
            float tr_w = bw2 - lbl_w2 - 52.0f * scale;

            draw_fault_rate_row(&ui_font, "WARNING", warning_pct,
                                bx2, tr_x, tr_w, bh2,
                                metrics_area.y + 134.0f * scale,
                                COL_WARNING_FILL, scale);
            draw_fault_rate_row(&ui_font, "SHUTDOWN", shutdown_pct,
                                bx2, tr_x, tr_w, bh2,
                                metrics_area.y + 148.0f * scale,
                                COL_SHUTDOWN_FILL, scale);
        }

        /* End of scenario notice */
        if (playhead >= (float)(scenario->tick_count - 1U))
        {
            DrawLine((int)(metrics_area.x + 14.0f * scale), (int)(metrics_area.y + 172.0f * scale),
                     (int)(metrics_area.x + metrics_area.width - 14.0f * scale),
                     (int)(metrics_area.y + 172.0f * scale),
                     COL_SECTION_DIV);
            draw_text_font(&ui_font, "End of scenario",
                           metrics_area.x + 14.0f * scale, metrics_area.y + 180.0f * scale,
                           FS_SMALL * scale, COL_END_NOTICE);
            draw_text_font(&ui_font, "Press  R  to replay",
                           metrics_area.x + 14.0f * scale, metrics_area.y + 194.0f * scale,
                           FS_SMALL * scale, COL_REPLAY_HINT);
        }

        /* ── TIMELINE + SLIDER ─────────────────────────── */
        draw_timeline(&ui_font, scenario, playhead, timeline, scale);
        draw_slider(&ui_font, slider, scenario, playhead, restart_feedback_timer);

        if (quit_modal_open != 0)
        {
            draw_quit_modal(&ui_font, quit_modal_selection, screen_w, screen_h, scale);
        }

        EndDrawing();
    }

    if (custom_font_loaded != 0)
    {
        UnloadFont(ui_font);
    }
    CloseWindow();
}

int main(int argc, char **argv)
{
    ScenarioSet scenario_set;

    if ((argc < 2) || (argv == NULL) || (argv[1] == NULL))
    {
        (void)fprintf(stderr, "Usage: %s <scenario.json> [more_scenarios.json ...]\n", (argc > 0) ? argv[0] : "visualizer");
        return 1;
    }

    if (load_scenarios_from_files(argc, argv, &scenario_set) == 0)
    {
        (void)fprintf(stderr, "Failed to load scenario JSON file(s).\n");
        return 1;
    }

    run_visualizer(&scenario_set);
    return 0;
}
