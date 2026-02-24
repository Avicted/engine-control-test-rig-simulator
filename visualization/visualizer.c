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

static float draw_key_hint(const Font *font,
                           const char *key,
                           const char *desc,
                           float x,
                           float y,
                           float font_size)
{
    Color key_color = (Color){90, 170, 255, 255};
    Color brak_color = (Color){50, 62, 94, 255};
    Color desc_color = (Color){110, 124, 155, 255};
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
    float lbl_fs = 13.0f * scale;
    float val_fs = 16.0f * scale;
    Vector2 value_size;
    float value_x;
    float value_y;

    ratio = (max_value > 0.0f) ? (value / max_value) : 0.0f;
    ratio = clamp01(ratio);
    fill_width = (int)(ratio * bar_area.width);

    draw_text_font(font, label, bar_area.x, bar_area.y - lbl_fs - 3.0f * scale, lbl_fs,
                   (Color){120, 134, 165, 255});
    DrawRectangleRec(bar_area, (Color){18, 22, 34, 255});
    if (fill_width > 0)
    {
        DrawRectangle((int)bar_area.x, (int)bar_area.y, fill_width, (int)bar_area.height, level_color(level));
    }
    DrawRectangleLines((int)bar_area.x, (int)bar_area.y, (int)bar_area.width, (int)bar_area.height,
                       (Color){40, 46, 66, 255});

    if ((warn_threshold > 0.0f) && (warn_threshold < max_value))
    {
        int warn_x = (int)(bar_area.x + (warn_threshold / max_value) * bar_area.width);
        DrawLine(warn_x, (int)(bar_area.y + 2), warn_x, (int)(bar_area.y + bar_area.height - 2),
                 (Color){255, 193, 7, 200});
    }
    if ((shutdown_threshold > 0.0f) && (shutdown_threshold < max_value))
    {
        int shut_x = (int)(bar_area.x + (shutdown_threshold / max_value) * bar_area.width);
        DrawLine(shut_x, (int)(bar_area.y + 2), shut_x, (int)(bar_area.y + bar_area.height - 2),
                 (Color){220, 53, 69, 210});
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

    DrawRectangleRec(area, (Color){12, 15, 24, 255});
    DrawRectangleLines((int)area.x, (int)area.y, (int)area.width, (int)area.height, (Color){32, 38, 58, 255});

    draw_text_font(font, "TIMELINE", area.x + 14.0f, area.y + 8.0f, 18.0f, (Color){72, 85, 120, 255});
    DrawLine((int)(area.x + 1), (int)(area.y + 32.0f),
             (int)(area.x + area.width - 1), (int)(area.y + 32.0f),
             (Color){26, 31, 50, 255});

    {
        unsigned int active_tick = scenario->ticks[(unsigned int)(playhead + 0.5f)].tick;
        char tick_label[48];
        Vector2 tl_sz;
        float tl_x;
        (void)snprintf(tick_label, sizeof(tick_label), "Tick %u / %u", active_tick,
                       scenario->ticks[scenario->tick_count - 1U].tick);
        tl_sz = MeasureTextEx(*font, tick_label, 18.0f, 1.0f);
        tl_x = area.x + area.width * 0.5f - tl_sz.x * 0.5f;
        draw_text_font(font, tick_label, tl_x, area.y + 8.0f, 18.0f, (Color){140, 155, 186, 255});
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
        draw_dashed_hline(plot_left, plot_right, temp_warn_y, 10, 5, (Color){255, 193, 7, 160});
        draw_dashed_hline(plot_left, plot_right, temp_shutdown_y, 10, 5, (Color){220, 53, 69, 170});
        draw_dashed_hline(plot_left, plot_right, oil_shutdown_y, 10, 5, (Color){220, 53, 69, 120});
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

            DrawLineEx((Vector2){(float)x0, (float)yr0}, (Vector2){(float)x1, (float)yr1}, 2.0f,
                       (Color){52, 152, 219, 255});
            DrawLineEx((Vector2){(float)x0, (float)yt0}, (Vector2){(float)x1, (float)yt1}, 2.0f,
                       (Color){255, 99, 132, 255});
            DrawLineEx((Vector2){(float)x0, (float)yo0}, (Vector2){(float)x1, (float)yo1}, 2.0f,
                       (Color){46, 204, 113, 255});
            DrawLineEx((Vector2){(float)x0, (float)yc0}, (Vector2){(float)x1, (float)yc1}, 2.0f,
                       (Color){176, 132, 255, 255});
        }

        {
            float marker_t = clamp01(playhead / (float)(scenario->tick_count - 1U));
            int marker_x = plot_left + (int)(marker_t * (float)plot_w);
            DrawLineEx((Vector2){(float)marker_x, (float)plot_top},
                       (Vector2){(float)marker_x, (float)plot_bottom},
                       2.0f, (Color){255, 255, 255, 210});
        }
    }

    /* Compact right-to-left legend with colour dots */
    {
        float ley = area.y + 8.0f;
        float lfs2 = 17.0f;
        float dot_r = 5.0f;
        float lx = area.x + area.width - 12.0f;
        Vector2 lsz;

        lsz = MeasureTextEx(*font, "CTRL", lfs2, 1.0f);
        lx -= lsz.x;
        draw_text_font(font, "CTRL", lx, ley, lfs2, (Color){176, 132, 255, 255});
        lx -= dot_r * 2.0f + 6.0f;
        DrawCircle((int)(lx + dot_r), (int)(ley + lfs2 * 0.5f), dot_r, (Color){176, 132, 255, 255});
        lx -= 18.0f;

        lsz = MeasureTextEx(*font, "OIL", lfs2, 1.0f);
        lx -= lsz.x;
        draw_text_font(font, "OIL", lx, ley, lfs2, (Color){46, 204, 113, 255});
        lx -= dot_r * 2.0f + 6.0f;
        DrawCircle((int)(lx + dot_r), (int)(ley + lfs2 * 0.5f), dot_r, (Color){46, 204, 113, 255});
        lx -= 18.0f;

        lsz = MeasureTextEx(*font, "TEMP", lfs2, 1.0f);
        lx -= lsz.x;
        draw_text_font(font, "TEMP", lx, ley, lfs2, (Color){255, 99, 132, 255});
        lx -= dot_r * 2.0f + 6.0f;
        DrawCircle((int)(lx + dot_r), (int)(ley + lfs2 * 0.5f), dot_r, (Color){255, 99, 132, 255});
        lx -= 18.0f;

        lsz = MeasureTextEx(*font, "RPM", lfs2, 1.0f);
        lx -= lsz.x;
        draw_text_font(font, "RPM", lx, ley, lfs2, (Color){52, 152, 219, 255});
        lx -= dot_r * 2.0f + 6.0f;
        DrawCircle((int)(lx + dot_r), (int)(ley + lfs2 * 0.5f), dot_r, (Color){52, 152, 219, 255});
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
        DrawRectangle(tx, cy - th / 2, tw, th, (Color){22, 27, 44, 255});
        DrawRectangleLines(tx, cy - th / 2, tw, th, (Color){44, 52, 76, 255});
        /* Filled progress */
        if (knob_t > 0.0f)
        {
            int fw = (int)(tw * knob_t);
            DrawRectangle(tx + rx, cy - th / 2, fw - rx, th, (Color){48, 128, 220, 255});
        }

        /* Knob: shadow ring, white fill, blue accent */
        DrawCircle(knob_x, cy, 11.0f, (Color){8, 10, 18, 180});
        DrawCircle(knob_x, cy, 9.0f, (Color){220, 228, 245, 255});
        DrawCircleLines(knob_x, cy, 11.0f, (Color){48, 128, 220, 200});
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
        int screen_w = GetScreenWidth();
        int screen_h = GetScreenHeight();
        float scale = screen_scale(screen_w, screen_h);
        float pad = 16.0f * scale;
        float hdr_h = 50.0f * scale;
        float nav_h = 30.0f * scale;
        float content_y = hdr_h + nav_h + 8.0f * scale;
        float main_h = 252.0f * scale;
        float status_panel_w = 310.0f * scale;
        float col_gap = 12.0f * scale;
        float gauges_x = pad;
        float gauges_w = (float)screen_w - (pad * 2.0f) - status_panel_w - col_gap;
        float status_x = gauges_x + gauges_w + col_gap;
        float gc_x = gauges_x + 14.0f * scale;
        float gc_w = gauges_w - 28.0f * scale;
        float bar_col_w = gc_w * 0.68f;
        float val_col_w = gc_w - bar_col_w - 10.0f * scale;
        float val_col_x = gc_x + bar_col_w + 10.0f * scale;
        float bar_h = 22.0f * scale;
        float row_step = 42.0f * scale;
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
                            timeline.y + timeline.height - 30.0f * scale,
                            timeline.width - 76.0f,
                            24.0f * scale};
        float warning_pct = 0.0f;
        float shutdown_pct = 0.0f;

        compute_cumulative_metrics(scenario, playhead, &warning_pct, &shutdown_pct);

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
        ClearBackground((Color){9, 11, 18, 255});
        DrawRectangleGradientV(0, 0, screen_w, screen_h, (Color){11, 14, 22, 255}, (Color){7, 9, 16, 255});

        /* ── HEADER BAR ────────────────────────────────── */
        DrawRectangle(0, 0, screen_w, (int)hdr_h, (Color){13, 16, 26, 255});
        DrawLine(0, (int)hdr_h - 1, screen_w, (int)hdr_h - 1, (Color){30, 36, 58, 255});
        {
            char scen_label[48];
            char tick_str[48];
            Vector2 tick_sz;
            float badge_w;
            float badge_x;
            float badge_y;
            float badge_h = 28.0f * scale;

            (void)snprintf(scen_label, sizeof(scen_label),
                           "SCENARIO  %u / %u",
                           scenario_set->active_index + 1U, scenario_set->count);
            draw_text_font(&ui_font, scen_label, pad, 7.0f * scale, 11.0f * scale,
                           (Color){78, 94, 136, 255});
            draw_text_font(&ui_font, scenario->scenario, pad, 21.0f * scale, 20.0f * scale,
                           RAYWHITE);

            (void)snprintf(tick_str, sizeof(tick_str), "Tick  %u / %u",
                           tick->tick, scenario->tick_count);
            tick_sz = MeasureTextEx(ui_font, tick_str, 15.0f * scale, 1.0f);
            badge_w = tick_sz.x + 20.0f * scale;
            badge_x = (float)screen_w - badge_w - pad;
            badge_y = (hdr_h - badge_h) * 0.5f;
            DrawRectangle((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h,
                          (Color){20, 25, 40, 255});
            DrawRectangleLines((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h,
                               (Color){44, 54, 84, 255});
            draw_text_font(&ui_font, tick_str,
                           badge_x + 10.0f * scale,
                           badge_y + (badge_h - 15.0f * scale) * 0.5f,
                           15.0f * scale, (Color){148, 165, 205, 255});
        }

        /* ── CONTROLS BAR ──────────────────────────────── */
        DrawRectangle(0, (int)hdr_h, screen_w, (int)nav_h, (Color){10, 12, 20, 255});
        DrawLine(0, (int)(hdr_h + nav_h), screen_w, (int)(hdr_h + nav_h), (Color){24, 29, 48, 255});
        {
            float kx = pad;
            float ky = hdr_h + (nav_h - 13.0f * scale) * 0.5f;
            float kfs = 13.0f * scale;
            float gap2 = 20.0f * scale;
            char speed_str[32];
            Vector2 sp_sz;

            kx += draw_key_hint(&ui_font, "SPC", "Pause", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "R", "Restart", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "UP DN", "Speed", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "LT RT", "Step", kx, ky, kfs) + gap2;
            kx += draw_key_hint(&ui_font, "TAB", "Switch", kx, ky, kfs);
            (void)kx;

            (void)snprintf(speed_str, sizeof(speed_str), "%.0f tk/s%s",
                           ticks_per_second, (paused != 0) ? "  PAUSED" : "");
            sp_sz = MeasureTextEx(ui_font, speed_str, kfs, 1.0f);
            draw_text_font(&ui_font, speed_str,
                           (float)screen_w - sp_sz.x - pad, ky, kfs,
                           (paused != 0) ? (Color){255, 193, 7, 220} : (Color){72, 86, 120, 255});
        }

        /* ── GAUGES PANEL ──────────────────────────────── */
        DrawRectangleRec(gauges_panel, (Color){13, 16, 26, 255});
        DrawRectangleLines((int)gauges_panel.x, (int)gauges_panel.y,
                           (int)gauges_panel.width, (int)gauges_panel.height,
                           (Color){30, 36, 56, 255});
        draw_text_font(&ui_font, "SENSOR READINGS",
                       gauges_panel.x + 14.0f * scale, gauges_panel.y + 10.0f * scale,
                       12.0f * scale, (Color){68, 82, 118, 255});
        DrawLine((int)(gauges_panel.x + 1), (int)(gauges_panel.y + 31.0f * scale),
                 (int)(gauges_panel.x + gauges_panel.width - 1), (int)(gauges_panel.y + 31.0f * scale),
                 (Color){24, 29, 48, 255});

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
        DrawRectangleRec(metrics_area, (Color){13, 16, 26, 255});
        DrawRectangleLines((int)metrics_area.x, (int)metrics_area.y,
                           (int)metrics_area.width, (int)metrics_area.height,
                           (Color){30, 36, 56, 255});
        draw_text_font(&ui_font, "ENGINE STATUS",
                       metrics_area.x + 14.0f * scale, metrics_area.y + 10.0f * scale,
                       12.0f * scale, (Color){68, 82, 118, 255});
        DrawLine((int)(metrics_area.x + 1), (int)(metrics_area.y + 31.0f * scale),
                 (int)(metrics_area.x + metrics_area.width - 1), (int)(metrics_area.y + 31.0f * scale),
                 (Color){24, 29, 48, 255});

        /* Mode (left) + Result/Run (right), same horizontal band */
        {
            float lx = metrics_area.x + 14.0f * scale;
            float rx = metrics_area.x + metrics_area.width * 0.54f;
            float cap_y = metrics_area.y + 38.0f * scale;
            float cap_fs = 11.0f * scale;
            float val_fs = 13.0f * scale;
            float mode_fs = 22.0f * scale;
            char run_val[16];
            Color result_c;

            result_c = (strcmp(tick->result, "OK") == 0)
                           ? (Color){46, 204, 113, 255}
                       : (strcmp(tick->result, "WARNING") == 0)
                           ? (Color){255, 193, 7, 255}
                           : (Color){220, 53, 69, 255};

            /* Left: MODE caption + big mode text */
            draw_text_font(&ui_font, "MODE", lx, cap_y, cap_fs, (Color){68, 82, 118, 255});
            draw_text_font(&ui_font, tick->engine_mode, lx, cap_y + cap_fs + 4.0f * scale,
                           mode_fs, animated_mode_color);

            /* Right: RESULT caption + value, then RUN caption + value */
            draw_text_font(&ui_font, "RESULT", rx, cap_y, cap_fs, (Color){68, 82, 118, 255});
            draw_text_font(&ui_font, tick->result, rx, cap_y + cap_fs + 4.0f * scale, val_fs, result_c);
            draw_text_font(&ui_font, "RUN", rx, cap_y + cap_fs + 4.0f * scale + val_fs + 10.0f * scale,
                           cap_fs, (Color){68, 82, 118, 255});
            (void)snprintf(run_val, sizeof(run_val), "%d", tick->run);
            draw_text_font(&ui_font, run_val,
                           rx, cap_y + cap_fs * 2.0f + val_fs + 8.0f * scale + 10.0f * scale,
                           val_fs, RAYWHITE);
        }

        /* Divider */
        DrawLine((int)(metrics_area.x + 14.0f * scale), (int)(metrics_area.y + 110.0f * scale),
                 (int)(metrics_area.x + metrics_area.width - 14.0f * scale),
                 (int)(metrics_area.y + 110.0f * scale),
                 (Color){24, 29, 48, 255});

        /* Session fault rate */
        draw_text_font(&ui_font, "SESSION FAULT RATE",
                       metrics_area.x + 14.0f * scale, metrics_area.y + 118.0f * scale,
                       11.0f * scale, (Color){68, 82, 118, 255});
        {
            float bx2 = metrics_area.x + 14.0f * scale;
            float bw2 = metrics_area.width - 28.0f * scale;
            float bh2 = 9.0f * scale;
            float lbl_w2 = 68.0f * scale;
            float tr_x = bx2 + lbl_w2;
            float tr_w = bw2 - lbl_w2 - 52.0f * scale;
            char warn_txt[20];
            char shut_txt[20];

            (void)snprintf(warn_txt, sizeof(warn_txt), "%.1f%%", warning_pct);
            (void)snprintf(shut_txt, sizeof(shut_txt), "%.1f%%", shutdown_pct);

            draw_text_font(&ui_font, "WARNING", bx2, metrics_area.y + 134.0f * scale,
                           11.0f * scale, (Color){90, 106, 148, 255});
            DrawRectangle((int)tr_x, (int)(metrics_area.y + 134.0f * scale),
                          (int)tr_w, (int)bh2, (Color){18, 22, 34, 255});
            DrawRectangle((int)tr_x, (int)(metrics_area.y + 134.0f * scale),
                          (int)(tr_w * (warning_pct / 100.0f)), (int)bh2,
                          (Color){255, 193, 7, 200});
            DrawRectangleLines((int)tr_x, (int)(metrics_area.y + 134.0f * scale),
                               (int)tr_w, (int)bh2, (Color){38, 44, 64, 255});
            draw_text_font(&ui_font, warn_txt, tr_x + tr_w + 6.0f * scale,
                           metrics_area.y + 134.0f * scale, 11.0f * scale,
                           (Color){170, 182, 212, 255});

            draw_text_font(&ui_font, "SHUTDOWN", bx2, metrics_area.y + 148.0f * scale,
                           11.0f * scale, (Color){90, 106, 148, 255});
            DrawRectangle((int)tr_x, (int)(metrics_area.y + 148.0f * scale),
                          (int)tr_w, (int)bh2, (Color){18, 22, 34, 255});
            DrawRectangle((int)tr_x, (int)(metrics_area.y + 148.0f * scale),
                          (int)(tr_w * (shutdown_pct / 100.0f)), (int)bh2,
                          (Color){220, 53, 69, 210});
            DrawRectangleLines((int)tr_x, (int)(metrics_area.y + 148.0f * scale),
                               (int)tr_w, (int)bh2, (Color){38, 44, 64, 255});
            draw_text_font(&ui_font, shut_txt, tr_x + tr_w + 6.0f * scale,
                           metrics_area.y + 148.0f * scale, 11.0f * scale,
                           (Color){170, 182, 212, 255});
        }

        /* End of scenario notice */
        if (playhead >= (float)(scenario->tick_count - 1U))
        {
            DrawLine((int)(metrics_area.x + 14.0f * scale), (int)(metrics_area.y + 172.0f * scale),
                     (int)(metrics_area.x + metrics_area.width - 14.0f * scale),
                     (int)(metrics_area.y + 172.0f * scale),
                     (Color){24, 29, 48, 255});
            draw_text_font(&ui_font, "End of scenario",
                           metrics_area.x + 14.0f * scale, metrics_area.y + 180.0f * scale,
                           12.0f * scale, (Color){255, 220, 100, 255});
            draw_text_font(&ui_font, "Press  R  to replay",
                           metrics_area.x + 14.0f * scale, metrics_area.y + 194.0f * scale,
                           12.0f * scale, (Color){160, 174, 210, 255});
        }

        /* ── TIMELINE + SLIDER ─────────────────────────── */
        draw_timeline(&ui_font, scenario, playhead, timeline, scale);
        draw_slider(&ui_font, slider, scenario, playhead, restart_feedback_timer);

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
