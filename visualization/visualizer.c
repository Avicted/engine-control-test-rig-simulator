#include <ctype.h>
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

    return (sscanf(value_start, "%u", out) == 1) ? 1 : 0;
}

static int parse_int_value(const char *object_start, const char *key, int *out)
{
    const char *key_pos;
    const char *value_start;

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

    return (sscanf(value_start, "%d", out) == 1) ? 1 : 0;
}

static int parse_float_value(const char *object_start, const char *key, float *out)
{
    const char *key_pos;
    const char *value_start;

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

    return (sscanf(value_start, "%f", out) == 1) ? 1 : 0;
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
    const char *scenarios_key;
    const char *array_start;
    const char *cursor;
    const char *array_end;
    unsigned int scenario_count;

    if ((json_text == NULL) || (scenarios == NULL) || (parsed_count == NULL) || (max_scenarios == 0U))
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
        return (Color){220, 53, 69, 255};
    }
    if (level == LEVEL_WARNING)
    {
        return (Color){255, 193, 7, 255};
    }
    return (Color){40, 167, 69, 255};
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
        return (Color){180, 186, 200, 255};
    }
    if ((strcmp(engine_mode, "INIT") == 0) || (strcmp(engine_mode, "STARTING") == 0))
    {
        return (Color){52, 152, 219, 255};
    }
    if (strcmp(engine_mode, "WARNING") == 0)
    {
        return (Color){255, 193, 7, 255};
    }
    if (strcmp(engine_mode, "SHUTDOWN") == 0)
    {
        return (Color){220, 53, 69, 255};
    }
    return (Color){40, 167, 69, 255};
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

static void draw_meter(const Font *font,
                       const char *label,
                       float value,
                       float max_value,
                       Rectangle area,
                       SeverityLevel level,
                       const char *unit,
                       float warn_threshold,
                       float shutdown_threshold,
                       float value_right_x)
{
    float ratio;
    int fill_width;
    char value_text[64];
    float value_font_size = 24.0f;
    Vector2 value_size;
    float value_x;

    ratio = (max_value > 0.0f) ? (value / max_value) : 0.0f;
    ratio = clamp01(ratio);
    fill_width = (int)(ratio * area.width);

    draw_text_font(font, label, area.x, area.y - 22.0f, 18.0f, RAYWHITE);
    DrawRectangleRec(area, (Color){30, 34, 44, 255});
    DrawRectangle((int)area.x, (int)area.y, fill_width, (int)area.height, level_color(level));
    DrawRectangleLines((int)area.x, (int)area.y, (int)area.width, (int)area.height, (Color){95, 100, 118, 255});

    if ((warn_threshold > 0.0f) && (warn_threshold < max_value))
    {
        int warn_x = (int)(area.x + (warn_threshold / max_value) * area.width);
        DrawLine(warn_x, (int)area.y, warn_x, (int)(area.y + area.height), (Color){255, 193, 7, 220});
    }
    if ((shutdown_threshold > 0.0f) && (shutdown_threshold < max_value))
    {
        int shut_x = (int)(area.x + (shutdown_threshold / max_value) * area.width);
        DrawLine(shut_x, (int)area.y, shut_x, (int)(area.y + area.height), (Color){220, 53, 69, 230});
    }

    (void)snprintf(value_text, sizeof(value_text), "%.2f %s", value, unit);
    value_size = MeasureTextEx(*font, value_text, value_font_size, 1.0f);
    value_x = value_right_x - value_size.x;
    if (value_x < (area.x + area.width + 10.0f))
    {
        value_x = area.x + area.width + 10.0f;
    }
    draw_text_font(font,
                   value_text,
                   value_x,
                   area.y + ((area.height - value_font_size) * 0.5f),
                   value_font_size,
                   RAYWHITE);
}

static void draw_timeline(const Font *font, const ScenarioData *scenario, float playhead, Rectangle area)
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

    DrawRectangleRec(area, (Color){15, 18, 25, 255});
    DrawRectangleLines((int)area.x, (int)area.y, (int)area.width, (int)area.height, (Color){95, 100, 118, 255});
    draw_text_font(font, "Timeline", area.x + 12.0f, area.y + 10.0f, 20.0f, RAYWHITE);

    {
        Vector2 title_size = MeasureTextEx(*font, "Timeline", 20.0f, 1.0f);
        unsigned int active_tick = scenario->ticks[(unsigned int)(playhead + 0.5f)].tick;
        char tick_label[48];
        (void)snprintf(tick_label, sizeof(tick_label), "Tick %u / %u", active_tick, scenario->ticks[scenario->tick_count - 1U].tick);
        draw_text_font(font,
                       tick_label,
                       area.x + 12.0f + title_size.x + 18.0f,
                       area.y + 11.0f,
                       16.0f,
                       (Color){170, 178, 194, 255});
    }

    plot_left = (int)area.x + 52;
    plot_right = (int)(area.x + area.width) - 24;
    plot_top = (int)area.y + 50;
    plot_bottom = (int)(area.y + area.height) - 74;
    plot_w = plot_right - plot_left;
    plot_h = plot_bottom - plot_top;

    for (i = 0; i <= 8; ++i)
    {
        int gy = plot_top + (i * plot_h) / 8;
        Color grid_color = (i == 8) ? (Color){96, 104, 122, 220} : (Color){44, 49, 63, 180};
        DrawLine(plot_left, gy, plot_right, gy, grid_color);
        if ((i % 2) == 0)
        {
            char y_label[16];
            (void)snprintf(y_label, sizeof(y_label), "%d%%", 100 - ((i * 100) / 8));
            draw_text_font(font, y_label, area.x + 8.0f, (float)gy - 8.0f, 14.0f, (Color){160, 168, 184, 255});
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
            DrawLine(gx, plot_top, gx, plot_bottom, (Color){38, 43, 58, 170});
            (void)snprintf(x_label, sizeof(x_label), "%u", scenario->ticks[tick_idx].tick);
            draw_text_font(font, x_label, (float)gx - 8.0f, (float)plot_bottom + 8.0f, 14.0f, (Color){160, 168, 184, 255});
        }
    }

    {
        int temp_warn_y = plot_bottom - (int)((TEMP_WARNING_THRESHOLD / 120.0f) * (float)plot_h);
        int temp_shutdown_y = plot_bottom - (int)((TEMP_SHUTDOWN_THRESHOLD / 120.0f) * (float)plot_h);
        int oil_shutdown_y = plot_bottom - (int)((OIL_SHUTDOWN_THRESHOLD / 5.0f) * (float)plot_h);
        DrawLine(plot_left, temp_warn_y, plot_right, temp_warn_y, (Color){255, 193, 7, 120});
        DrawLine(plot_left, temp_shutdown_y, plot_right, temp_shutdown_y, (Color){220, 53, 69, 130});
        DrawLine(plot_left, oil_shutdown_y, plot_right, oil_shutdown_y, (Color){220, 53, 69, 90});
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
                DrawRectangle(sx, plot_top, ex - sx, plot_h, (Color){255, 193, 7, 26});
            }
            else if (level == LEVEL_SHUTDOWN)
            {
                DrawRectangle(sx, plot_top, ex - sx, plot_h, (Color){220, 53, 69, 34});
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

            DrawLine(x0, yr0, x1, yr1, (Color){52, 152, 219, 255});
            DrawLine(x0, yt0, x1, yt1, (Color){255, 99, 132, 255});
            DrawLine(x0, yo0, x1, yo1, (Color){46, 204, 113, 255});
            DrawLine(x0, yc0, x1, yc1, (Color){176, 132, 255, 255});
        }

        {
            float marker_t = clamp01(playhead / (float)(scenario->tick_count - 1U));
            int marker_x = plot_left + (int)(marker_t * (float)plot_w);
            DrawLine(marker_x, plot_top, marker_x, plot_bottom, (Color){255, 255, 255, 190});
        }
    }

    draw_text_font(font, "RPM", area.x + area.width - 286.0f, area.y + 12.0f, 16.0f, (Color){52, 152, 219, 255});
    draw_text_font(font, "TEMP", area.x + area.width - 238.0f, area.y + 12.0f, 16.0f, (Color){255, 99, 132, 255});
    draw_text_font(font, "OIL", area.x + area.width - 180.0f, area.y + 12.0f, 16.0f, (Color){46, 204, 113, 255});
    draw_text_font(font, "CTRL", area.x + area.width - 132.0f, area.y + 12.0f, 16.0f, (Color){176, 132, 255, 255});
    draw_text_font(font,
                   "Fault Overlay: WARNING/SHUTDOWN",
                   area.x + 12.0f,
                   area.y + area.height - 26.0f,
                   14.0f,
                   (Color){190, 196, 210, 255});
}

static void draw_slider(const Font *font,
                        Rectangle slider,
                        const ScenarioData *scenario,
                        float playhead,
                        float restart_feedback_timer)
{
    float knob_t;
    float pulse;
    int knob_x;

    if ((scenario == NULL) || (scenario->tick_count == 0U))
    {
        return;
    }

    (void)font;

    DrawRectangleRec(slider, (Color){32, 36, 48, 255});
    DrawRectangleLines((int)slider.x, (int)slider.y, (int)slider.width, (int)slider.height, (Color){96, 104, 122, 255});

    knob_t = (scenario->tick_count > 1U) ? clamp01(playhead / (float)(scenario->tick_count - 1U)) : 0.0f;
    DrawRectangle((int)slider.x,
                  (int)slider.y,
                  (int)(slider.width * knob_t),
                  (int)slider.height,
                  (Color){83, 168, 255, 70});
    knob_x = (int)(slider.x + (slider.width * knob_t));
    DrawCircle(knob_x, (int)(slider.y + slider.height * 0.5f), 8.0f, RAYWHITE);

    pulse = clamp01(restart_feedback_timer / 0.6f);
    if (pulse > 0.0f)
    {
        Color flash = (Color){80, 180, 255, (unsigned char)(70 + (int)(150.0f * pulse))};
        DrawRectangle((int)slider.x, (int)slider.y, (int)slider.width, (int)slider.height, flash);
    }
}

static void compute_cumulative_metrics(const ScenarioData *scenario, float *warning_pct, float *shutdown_pct)
{
    unsigned int i;
    unsigned int warn_count = 0U;
    unsigned int shut_count = 0U;

    if ((scenario == NULL) || (warning_pct == NULL) || (shutdown_pct == NULL) || (scenario->tick_count == 0U))
    {
        return;
    }

    for (i = 0U; i < scenario->tick_count; ++i)
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

    *warning_pct = (100.0f * (float)warn_count) / (float)scenario->tick_count;
    *shutdown_pct = (100.0f * (float)shut_count) / (float)scenario->tick_count;
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

    while (!WindowShouldClose())
    {
        ScenarioData *scenario = &scenario_set->scenarios[scenario_set->active_index];
        TickData interpolated_tick;
        const TickData *tick;
        char header[196];
        char control_text[64];
        char run_text[64];
        char metric_text[96];
        int screen_w = GetScreenWidth();
        int screen_h = GetScreenHeight();
        float scale = screen_scale(screen_w, screen_h);
        float pad = 26.0f * scale;
        float top_h = 300.0f * scale;
        float panel_w = ((float)screen_w - (pad * 3.0f)) * 0.47f;
        float status_x = pad + panel_w + (pad * 2.2f);
        float status_w = (float)screen_w - status_x - pad;
        float meter_value_right = status_x - (36.0f * scale);
        float meter_bar_left = pad + 12.0f * scale;
        float meter_bar_right = meter_value_right - (132.0f * scale);
        float meter_bar_width = meter_bar_right - meter_bar_left;
        if (meter_bar_width < (140.0f * scale))
        {
            meter_bar_width = 140.0f * scale;
        }
        Rectangle timeline = {pad, top_h + (pad * 1.1f), (float)screen_w - (pad * 2.0f), (float)screen_h - top_h - (pad * 2.3f)};
        Rectangle slider = {timeline.x + 52.0f,
                            timeline.y + timeline.height - 34.0f * scale,
                            timeline.width - 76.0f,
                            12.0f * scale};
        Rectangle rpm_area = {meter_bar_left, 116.0f * scale, meter_bar_width, 22.0f * scale};
        Rectangle temp_area = {meter_bar_left, 166.0f * scale, meter_bar_width, 22.0f * scale};
        Rectangle oil_area = {meter_bar_left, 216.0f * scale, meter_bar_width, 22.0f * scale};
        Rectangle metrics_area = {status_x, 96.0f * scale, status_w, 220.0f * scale};
        float warning_pct = 0.0f;
        float shutdown_pct = 0.0f;

        compute_cumulative_metrics(scenario, &warning_pct, &shutdown_pct);

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
        ClearBackground((Color){10, 12, 18, 255});

        DrawRectangleGradientV(0, 0, screen_w, screen_h, (Color){11, 15, 24, 255}, (Color){8, 10, 16, 255});

        (void)snprintf(header,
                       sizeof(header),
                       "Scenario %u/%u: %s  |  Tick %u / %u",
                       scenario_set->active_index + 1U,
                       scenario_set->count,
                       scenario->scenario,
                       tick->tick,
                       scenario->tick_count);
        draw_text_font(&ui_font, header, pad, 20.0f * scale, 24.0f * scale, RAYWHITE);

        draw_text_font(&ui_font,
                       "SPACE pause  R restart  UP/DOWN speed  LEFT/RIGHT step  TAB switch scenario",
                       pad,
                       54.0f * scale,
                       16.0f * scale,
                       (Color){180, 186, 200, 255});

        draw_meter(&ui_font,
                   "RPM",
                   tick->rpm,
                   5000.0f,
                   rpm_area,
                   rpm_to_level(tick->rpm, tick->temp),
                   "rpm",
                   RPM_WARNING_THRESHOLD,
                   0.0f,
                   meter_value_right);
        draw_meter(&ui_font,
                   "Temperature",
                   tick->temp,
                   120.0f,
                   temp_area,
                   temp_to_level(tick->temp),
                   "C",
                   TEMP_WARNING_THRESHOLD,
                   TEMP_SHUTDOWN_THRESHOLD,
                   meter_value_right);
        draw_meter(&ui_font,
                   "Oil Pressure",
                   tick->oil,
                   5.0f,
                   oil_area,
                   oil_to_level(tick->oil),
                   "bar",
                   0.0f,
                   OIL_SHUTDOWN_THRESHOLD,
                   meter_value_right);

        DrawRectangleRec(metrics_area, (Color){18, 21, 31, 255});
        DrawRectangleLines((int)metrics_area.x,
                           (int)metrics_area.y,
                           (int)metrics_area.width,
                           (int)metrics_area.height,
                           (Color){95, 100, 118, 255});

        draw_text_font(&ui_font, "Engine Mode", metrics_area.x + 20.0f * scale, metrics_area.y + 18.0f * scale, 22.0f * scale, RAYWHITE);
        draw_text_font(&ui_font,
                       tick->engine_mode,
                       metrics_area.x + 20.0f * scale,
                       metrics_area.y + 60.0f * scale,
                       36.0f * scale,
                       animated_mode_color);

        (void)snprintf(control_text,
                       sizeof(control_text),
                       "Result: %s",
                       tick->result);

        draw_text_font(&ui_font,
                       control_text,
                       metrics_area.x + 20.0f * scale,
                       metrics_area.y + 112.0f * scale,
                       18.0f * scale,
                       RAYWHITE);

        (void)snprintf(run_text, sizeof(run_text), "Run: %d", tick->run);
        draw_text_font(&ui_font,
                       run_text,
                       metrics_area.x + 20.0f * scale,
                       metrics_area.y + 132.0f * scale,
                       17.0f * scale,
                       (Color){200, 205, 218, 255});

        (void)snprintf(metric_text, sizeof(metric_text), "WARNING: %.1f%%   SHUTDOWN: %.1f%%", warning_pct, shutdown_pct);
        draw_text_font(&ui_font,
                       metric_text,
                       metrics_area.x + 20.0f * scale,
                       metrics_area.y + 152.0f * scale,
                       16.0f * scale,
                       (Color){200, 205, 218, 255});

        DrawRectangle((int)(metrics_area.x + 20.0f * scale),
                      (int)(metrics_area.y + 172.0f * scale),
                      (int)((metrics_area.width - 40.0f * scale) * (warning_pct / 100.0f)),
                      (int)(8.0f * scale),
                      (Color){255, 193, 7, 220});
        DrawRectangle((int)(metrics_area.x + 20.0f * scale),
                      (int)(metrics_area.y + 184.0f * scale),
                      (int)((metrics_area.width - 40.0f * scale) * (shutdown_pct / 100.0f)),
                      (int)(8.0f * scale),
                      (Color){220, 53, 69, 230});

        draw_timeline(&ui_font, scenario, playhead, timeline);
        draw_slider(&ui_font, slider, scenario, playhead, restart_feedback_timer);

        if (playhead >= (float)(scenario->tick_count - 1U))
        {
            draw_text_font(&ui_font,
                           "End of scenario - press R to replay",
                           metrics_area.x + 20.0f * scale,
                           metrics_area.y + metrics_area.height - 24.0f * scale,
                           15.0f * scale,
                           (Color){255, 220, 130, 255});
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
