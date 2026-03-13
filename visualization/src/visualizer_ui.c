#include <math.h>
#include <stdio.h>
#include <string.h>

#include "visualizer_config.h"
#include "visualizer_playback.h"
#include "visualizer_ui.h"

#define THEME (visualizer_active_theme())

/*=========================================================
THEME COLORS
=========================================================*/

/* Background layers */
#define COL_WINDOW_BG       (THEME->window_bg)
#define COL_GRAD_TOP        (THEME->grad_top)
#define COL_GRAD_BOT        (THEME->grad_bot)
#define COL_PANEL_BG        (THEME->panel_bg)
#define COL_NAV_BG          (THEME->nav_bg)
#define COL_TIMELINE_BG     (THEME->timeline_bg)
#define COL_BAR_BG          (THEME->bar_bg)
#define COL_BADGE_BG        (THEME->badge_bg)
#define COL_SLIDER_TRACK_BG (THEME->slider_track_bg)
#define COL_KNOB_SHADOW     (THEME->knob_shadow)

/* Borders / dividers */
#define COL_HDR_BORDER       (THEME->hdr_border)
#define COL_PANEL_BORDER     (THEME->panel_border)
#define COL_TIMELINE_BORDER  (THEME->timeline_border)
#define COL_SECTION_DIV      (THEME->section_div)
#define COL_TIMELINE_SUBDIV  (THEME->timeline_subdiv)
#define COL_FAULT_BAR_BOR    (THEME->fault_bar_bor)
#define COL_BAR_BORDER       (THEME->bar_border)
#define COL_SLIDER_TRACK_BOR (THEME->slider_track_bor)
#define COL_BADGE_BORDER     (THEME->badge_border)

/* Grid */
#define COL_GRID_LINE   (THEME->grid_line)
#define COL_GRID_BRIGHT (THEME->grid_bright)
#define COL_GRID_VERT   (THEME->grid_vert)

/* Text - dark to light */
#define COL_CAPTION        (THEME->caption)
#define COL_TIMELINE_TITLE (THEME->timeline_title)
#define COL_SPEED_RUNNING  (THEME->speed_running)
#define COL_SCEN_COUNTER   (THEME->scen_counter)
#define COL_SUBLABEL       (THEME->sublabel)
#define COL_KEY            (THEME->key)
#define COL_KEY_DESC       (THEME->key_desc)
#define COL_METER_LABEL    (THEME->meter_label)
#define COL_TICK_COUNTER   (THEME->tick_counter)
#define COL_BADGE_TEXT     (THEME->badge_text)
#define COL_AXIS_LABEL     (THEME->axis_label)
#define COL_FAULT_PCT_TEXT (THEME->fault_pct_text)
#define COL_PRIMARY_TEXT   (THEME->primary_text)
#define COL_KNOB_FILL      (THEME->knob_fill)
#define COL_MODE_DEFAULT   (THEME->mode_default)
#define COL_BRAK           (THEME->brak)

/* Semantic status */
#define COL_OK            (THEME->ok)
#define COL_WARNING       (THEME->warning)
#define COL_SHUTDOWN      (THEME->shutdown)
#define COL_WARNING_FILL  (THEME->warning_fill)
#define COL_SHUTDOWN_FILL (THEME->shutdown_fill)
#define COL_WARNING_DASH  (THEME->warning_dash)
#define COL_SHUTDOWN_DASH (THEME->shutdown_dash)
#define COL_OIL_SHUT_DASH (THEME->oil_shut_dash)
#define COL_WARNING_TINT  (THEME->warning_tint)
#define COL_SHUTDOWN_TINT (THEME->shutdown_tint)
#define COL_SPEED_PAUSED  (THEME->speed_paused)
#define COL_END_NOTICE    (THEME->end_notice)
#define COL_PLAYHEAD      (THEME->playhead)

/* Signal / sensor lines */
#define COL_RPM  (THEME->rpm)
#define COL_TEMP (THEME->temp)
#define COL_OIL  (THEME->oil)
#define COL_CTRL (THEME->ctrl)

/* Slider accent */
#define COL_SLIDER_FILL (THEME->slider_fill)
#define COL_SLIDER_RING (THEME->slider_ring)
#define COL_REPLAY_HINT (THEME->replay_hint)

#define PIXEL_FONT_HEIGHT 14.0f
#define PIXEL_FONT_SPACING 0.0f

static Color result_color(const char *result)
{
    if (strcmp(result, "OK") == 0) {
        return visualizer_active_theme()->oil;
    }
    if (strcmp(result, "WARNING") == 0) {
        return visualizer_active_theme()->warning;
    }

    return visualizer_active_theme()->shutdown;
}
static void draw_text_font(const Font *font, const char *text, float x, float y, float size, Color color)
{
    float snapped_size = 0.0f;

    if ((font == NULL) || (text == NULL)) {
        return;
    }

    snapped_size = PIXEL_FONT_HEIGHT * roundf(size / PIXEL_FONT_HEIGHT);
    if (snapped_size < PIXEL_FONT_HEIGHT) {
        snapped_size = PIXEL_FONT_HEIGHT;
    }

    DrawTextEx(*font, text, (Vector2){roundf(x), roundf(y)}, snapped_size, PIXEL_FONT_SPACING, color);
}

static Color with_min_alpha(Color color, unsigned char min_alpha)
{
    if (color.a < min_alpha) {
        color.a = min_alpha;
    }

    return color;
}

static Vector2 measure_text_font(const Font *font, const char *text, float size)
{
    float snapped_size = 0.0f;

    if ((font == NULL) || (text == NULL)) {
        return (Vector2){0.0f, 0.0f};
    }

    snapped_size = PIXEL_FONT_HEIGHT * roundf(size / PIXEL_FONT_HEIGHT);
    if (snapped_size < PIXEL_FONT_HEIGHT) {
        snapped_size = PIXEL_FONT_HEIGHT;
    }

    return MeasureTextEx(*font, text, snapped_size, PIXEL_FONT_SPACING);
}

static float pick_header_name_font_size(const Font *font,
                    const char *scenario_name,
                    const char *requirement_id,
                    const char *expected,
                    float available_width,
                    float scale)
{
    char expected_text[64] = {0};
    Vector2 name_sz = {0};
    Vector2 req_sz = {0};
    Vector2 exp_sz = {0};
    float preferred_fs = FS_SCEN_NAME * scale;
    float compact_fs = FS_TINY * scale;

    if ((font == NULL) || (scenario_name == NULL) || (requirement_id == NULL) || (expected == NULL)) {
        return compact_fs;
    }

    (void)snprintf(expected_text, sizeof(expected_text), "EXPECTED: %s", expected);
    name_sz = measure_text_font(font, scenario_name, preferred_fs);
    req_sz = measure_text_font(font, requirement_id, compact_fs);
    exp_sz = measure_text_font(font, expected_text, compact_fs);

    if ((name_sz.x + LAYOUT_HEADER_SEGMENT_GAP * scale + req_sz.x + LAYOUT_HEADER_META_GAP * scale + exp_sz.x) <=
        available_width) {
        return preferred_fs;
    }

    return compact_fs;
}

static Color level_color(SeverityLevel level)
{
    if (level == LEVEL_SHUTDOWN) {
        return COL_SHUTDOWN;
    }
    if (level == LEVEL_WARNING) {
        return COL_WARNING;
    }
    return COL_OK;
}

static unsigned int active_tick_index(const ScenarioData *scenario, float playhead)
{
    unsigned int index = 0U;

    if ((scenario == NULL) || (scenario->tick_count == 0U)) {
        return 0U;
    }

    index = (unsigned int)(playhead + 0.5f);
    if (index >= scenario->tick_count) {
        index = scenario->tick_count - 1U;
    }

    return index;
}

static void draw_threshold_marker(Rectangle bar_area, float threshold_ratio, SeverityLevel level, float scale)
{
    int marker_x = 0;
    int border_w = 0;
    int rect_w = 0;
    int rect_h = 0;
    int top_y = 0;
    int rect_x = 0;
    int gap_y = 0;
    Color border_color = with_min_alpha(COL_BAR_BORDER, 255U);
    Color fill_color = (Color){0};

    if ((threshold_ratio <= 0.0f) || (threshold_ratio >= 1.0f)) {
        return;
    }

    marker_x = (int)roundf(bar_area.x + (threshold_ratio * bar_area.width));
    border_w = ((int)roundf(scale) < 1) ? 1 : (int)roundf(scale);
    rect_w = 8 + ((int)roundf(scale) * 2);
    rect_h = 4 + border_w;
    gap_y = 2 + border_w;
    rect_x = marker_x - (rect_w / 2);
    top_y = (int)roundf(bar_area.y) - rect_h - gap_y;
    fill_color = (level == LEVEL_SHUTDOWN) ? with_min_alpha(COL_SHUTDOWN, 235U) : with_min_alpha(COL_WARNING, 235U);

    DrawRectangle(
        rect_x - border_w, top_y - border_w, rect_w + (border_w * 2), rect_h + (border_w * 2), border_color);
    DrawRectangle(rect_x, top_y, rect_w, rect_h, fill_color);
}

Color visualizer_mode_color(const char *engine_mode)
{
    if (engine_mode == NULL) {
        return COL_MODE_DEFAULT;
    }
    if ((strcmp(engine_mode, "INIT") == 0) || (strcmp(engine_mode, "STARTING") == 0)) {
        return COL_RPM;
    }
    if (strcmp(engine_mode, "WARNING") == 0) {
        return COL_WARNING;
    }
    if (strcmp(engine_mode, "SHUTDOWN") == 0) {
        return COL_SHUTDOWN;
    }
    return COL_OK;
}

Color visualizer_lerp_color(Color a, Color b, float t)
{
    Color out = (Color){0};
    float tt = visualizer_clamp01(t);

    out.r = (unsigned char)roundf(((float)a.r) + ((((float)b.r) - ((float)a.r)) * tt));
    out.g = (unsigned char)roundf(((float)a.g) + ((((float)b.g) - ((float)a.g)) * tt));
    out.b = (unsigned char)roundf(((float)a.b) + ((((float)b.b) - ((float)a.b)) * tt));
    out.a = (unsigned char)roundf(((float)a.a) + ((((float)b.a) - ((float)a.a)) * tt));
    return out;
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
    Vector2 sz = (Vector2){0};
    Color yes_bg = (Color){0};
    Color yes_bor = (Color){0};
    Color yes_txt = (Color){0};
    Color no_bg = (Color){0};
    Color no_bor = (Color){0};
    Color no_txt = (Color){0};

    DrawRectangle(0, 0, screen_w, screen_h, (Color){0, 0, 0, 160});
    DrawRectangle((int)(box_x + 4.0f), (int)(box_y + 4.0f), (int)box_w, (int)box_h, (Color){0, 0, 0, 110});
    DrawRectangle((int)box_x, (int)box_y, (int)box_w, (int)box_h, COL_PANEL_BG);
    DrawRectangleLines((int)box_x, (int)box_y, (int)box_w, (int)box_h, COL_PANEL_BORDER);

    sz = measure_text_font(font, title, title_fs);
    draw_text_font(font, title, box_x + (box_w - sz.x) * 0.5f, box_y + 14.0f * scale, title_fs, COL_PRIMARY_TEXT);

    DrawLine((int)(box_x + 16.0f * scale),
         (int)(box_y + 54.0f * scale),
         (int)(box_x + box_w - 16.0f * scale),
         (int)(box_y + 54.0f * scale),
         COL_SECTION_DIV);

    yes_bg = COL_BADGE_BG;
    yes_bor = (selection == 1) ? COL_PRIMARY_TEXT : COL_BADGE_BORDER;
    yes_txt = COL_PRIMARY_TEXT;
    DrawRectangle((int)yes_x, (int)btn_y, (int)btn_w, (int)btn_h, yes_bg);
    DrawRectangleLines((int)yes_x, (int)btn_y, (int)btn_w, (int)btn_h, yes_bor);
    sz = measure_text_font(font, "YES", opt_fs);
    draw_text_font(font, "YES", yes_x + (btn_w - sz.x) * 0.5f, btn_y + (btn_h - sz.y) * 0.5f, opt_fs, yes_txt);

    no_bg = COL_BADGE_BG;
    no_bor = (selection == 0) ? COL_PRIMARY_TEXT : COL_BADGE_BORDER;
    no_txt = COL_PRIMARY_TEXT;
    DrawRectangle((int)no_x, (int)btn_y, (int)btn_w, (int)btn_h, no_bg);
    DrawRectangleLines((int)no_x, (int)btn_y, (int)btn_w, (int)btn_h, no_bor);
    sz = measure_text_font(font, "NO", opt_fs);
    draw_text_font(font, "NO", no_x + (btn_w - sz.x) * 0.5f, btn_y + (btn_h - sz.y) * 0.5f, opt_fs, no_txt);

    sz = measure_text_font(font, hint, hint_fs);
    draw_text_font(font, hint, box_x + (box_w - sz.x) * 0.5f, box_y + box_h - 20.0f * scale, hint_fs, COL_SUBLABEL);
}

static float draw_key_hint(const Font *font, const char *key, const char *desc, float x, float y, float font_size)
{
    float cx = x;
    float w = 0.0f;

    w = measure_text_font(font, "[", font_size).x;
    draw_text_font(font, "[", cx, y, font_size, COL_BRAK);
    cx += w;
    w = measure_text_font(font, key, font_size).x;
    draw_text_font(font, key, cx, y, font_size, COL_KEY);
    cx += w;
    w = measure_text_font(font, "]", font_size).x;
    draw_text_font(font, "]", cx, y, font_size, COL_BRAK);
    cx += w + 4.0f;
    w = measure_text_font(font, desc, font_size).x;
    draw_text_font(font, desc, cx, y, font_size, COL_KEY_DESC);
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
    float ratio = 0.0f;
    int fill_width = 0;
    char value_text[64] = {0};
    float lbl_fs = FS_KEY_HINT * scale;
    float val_fs = FS_VALUE * scale;
    Vector2 value_size = (Vector2){0};
    float value_x = 0.0f;
    float value_y = 0.0f;

    ratio = (max_value > 0.0f) ? (value / max_value) : 0.0f;
    ratio = visualizer_clamp01(ratio);
    fill_width = (int)(ratio * bar_area.width);

    draw_text_font(font, label, bar_area.x, bar_area.y - lbl_fs - 3.0f * scale, lbl_fs, COL_METER_LABEL);
    DrawRectangleRec(bar_area, COL_BAR_BG);
    if (fill_width > 0) {
        DrawRectangle((int)bar_area.x, (int)bar_area.y, fill_width, (int)bar_area.height, level_color(level));
    }
    DrawRectangleLines((int)bar_area.x, (int)bar_area.y, (int)bar_area.width, (int)bar_area.height, COL_BAR_BORDER);

    if ((warn_threshold > 0.0f) && (warn_threshold < max_value)) {
        draw_threshold_marker(bar_area, warn_threshold / max_value, LEVEL_WARNING, scale);
    }
    if ((shutdown_threshold > 0.0f) && (shutdown_threshold < max_value)) {
        draw_threshold_marker(bar_area, shutdown_threshold / max_value, LEVEL_SHUTDOWN, scale);
    }

    (void)snprintf(value_text, sizeof(value_text), "%.2f %s", value, unit);
    value_size = measure_text_font(font, value_text, val_fs);
    value_x = value_area.x + value_area.width - value_size.x;
    if (value_x < value_area.x) {
        value_x = value_area.x;
    }
    value_y = bar_area.y + (bar_area.height - val_fs) * 0.5f;
    draw_text_font(font, value_text, value_x, value_y, val_fs, COL_PRIMARY_TEXT);
}

static void draw_dotted_hline(int x0, int x1, int y, int step, float radius, Color color)
{
    int x = x0;
    Color stroke = with_min_alpha(color, 160U);

    while (x < x1) {
        DrawCircle(x, y, radius, stroke);
        x += step;
    }
}

static float draw_panel_header(const Font *font, Rectangle area, const char *title, float scale)
{
    float title_y = area.y + 10.0f * scale;
    float divider_y = area.y + 31.0f * scale;

    DrawRectangleRec(area, COL_PANEL_BG);
    DrawRectangleLines((int)area.x, (int)area.y, (int)area.width, (int)area.height, COL_PANEL_BORDER);
    draw_text_font(font, title, area.x + 14.0f * scale, title_y, FS_SMALL * scale, COL_CAPTION);
    DrawLine((int)(area.x + 1), (int)divider_y, (int)(area.x + area.width - 1), (int)divider_y, COL_SECTION_DIV);

    return divider_y;
}

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
    char pct_text[20] = {0};
    float label_y = 0.0f;
    float pct_y = 0.0f;
    float label_fs = FS_TINY * scale;
    Vector2 label_sz = (Vector2){0};
    Vector2 pct_sz = (Vector2){0};
    float text_x = bar_x + bar_w + 6.0f * scale;

    label_sz = measure_text_font(font, label, label_fs);
    label_y = row_y + (bar_h - label_sz.y) * 0.5f;
    draw_text_font(font, label, label_x, label_y, label_fs, COL_SUBLABEL);

    DrawRectangle((int)bar_x, (int)row_y, (int)bar_w, (int)bar_h, COL_BAR_BG);
    DrawRectangle((int)bar_x, (int)row_y, (int)(bar_w * (value_pct / 100.0f)), (int)bar_h, fill_color);
    DrawRectangleLines((int)bar_x, (int)row_y, (int)bar_w, (int)bar_h, COL_FAULT_BAR_BOR);

    (void)snprintf(pct_text, sizeof(pct_text), "%.1f%%", value_pct);
    pct_sz = measure_text_font(font, pct_text, label_fs);
    pct_y = row_y + (bar_h - pct_sz.y) * 0.5f;
    draw_text_font(font, pct_text, text_x, pct_y, label_fs, COL_FAULT_PCT_TEXT);
}

typedef struct {
    int left;
    int right;
    int top;
    int bottom;
    int width;
    int height;
} TimelinePlotBounds;

static TimelinePlotBounds timeline_plot_bounds(Rectangle plot_area)
{
    TimelinePlotBounds bounds = {0};

    bounds.left = (int)roundf(plot_area.x);
    bounds.right = (int)roundf(plot_area.x + plot_area.width);
    bounds.top = (int)roundf(plot_area.y);
    bounds.bottom = (int)roundf(plot_area.y + plot_area.height);
    bounds.width = bounds.right - bounds.left;
    bounds.height = bounds.bottom - bounds.top;

    return bounds;
}

static void
draw_timeline_heading(const Font *font, const ScenarioData *scenario, float playhead, Rectangle area, float scale)
{
    unsigned int tick_index = active_tick_index(scenario, playhead);
    char tick_label[48] = {0};
    Vector2 tick_size = (Vector2){0};
    float title_fs = FS_TL_TITLE * scale;
    float tick_x = 0.0f;

    draw_text_font(font,
               "TIMELINE",
                area.x + TL_TITLE_X * scale,
                area.y + TL_TITLE_Y * scale,
               title_fs,
               COL_TIMELINE_TITLE);
    DrawLine((int)(area.x + 1),
            (int)(area.y + TL_HEADER_H * scale),
         (int)(area.x + area.width - 1),
            (int)(area.y + TL_HEADER_H * scale),
         COL_TIMELINE_SUBDIV);

    (void)snprintf(tick_label, sizeof(tick_label), "Tick %u / %u", tick_index + 1U, scenario->tick_count);
    tick_size = measure_text_font(font, tick_label, title_fs);
    tick_x = area.x + area.width * 0.5f - tick_size.x * 0.5f;
    draw_text_font(font, tick_label, tick_x, area.y + TL_TITLE_Y * scale, title_fs, COL_TICK_COUNTER);
}

static void draw_timeline_y_axis(const Font *font, const TimelinePlotBounds *bounds, Rectangle area, float scale)
{
    int i = 0;
    float axis_fs = FS_TL_AXIS * scale;

    for (i = 0; i <= 8; ++i) {
        int grid_y = bounds->top + (i * bounds->height) / 8;
        Color grid_color = (i == 8) ? COL_GRID_BRIGHT : COL_GRID_LINE;

        DrawLine(bounds->left, grid_y, bounds->right, grid_y, grid_color);
        if ((i % 2) != 0) {
            continue;
        }

        {
            char y_label[16] = {0};

            (void)snprintf(y_label, sizeof(y_label), "%d%%", 100 - ((i * 100) / 8));
            draw_text_font(font,
                       y_label,
                       area.x + LAYOUT_TIMELINE_AXIS_X * scale,
                       (float)grid_y - LAYOUT_TIMELINE_AXIS_Y_NUDGE * scale,
                       axis_fs,
                       COL_AXIS_LABEL);
        }
    }
}

static void
draw_timeline_x_axis(const Font *font, const TimelinePlotBounds *bounds, const ScenarioData *scenario, float scale)
{
    float axis_fs = FS_TL_AXIS * scale;

    if (scenario->tick_count <= 1U) {
        return;
    }

    if (scenario->tick_count > 32U) {
        unsigned int tick_idx = 0U;

        for (tick_idx = 0U; tick_idx < scenario->tick_count; tick_idx += 2U) {
            int grid_x = bounds->left + (int)roundf(((float)tick_idx / (float)(scenario->tick_count - 1U)) *
                                (float)bounds->width);
            char x_label[16] = {0};
            Vector2 label_size = (Vector2){0};

            DrawLine(grid_x, bounds->top, grid_x, bounds->bottom, COL_GRID_VERT);
            (void)snprintf(x_label, sizeof(x_label), "%u", tick_idx + 1U);
            label_size = measure_text_font(font, x_label, axis_fs);
            draw_text_font(font,
                       x_label,
                       (float)grid_x - label_size.x * 0.5f,
                       (float)bounds->bottom + LAYOUT_TIMELINE_XLABEL_Y * scale,
                       axis_fs,
                       COL_AXIS_LABEL);
        }

        if (((scenario->tick_count - 1U) % 2U) == 0U) {
            return;
        }

        {
            unsigned int last_tick_idx = scenario->tick_count - 1U;
            int grid_x =
                bounds->left + (int)roundf(((float)last_tick_idx / (float)(scenario->tick_count - 1U)) *
                               (float)bounds->width);
            char x_label[16] = {0};
            Vector2 label_size = (Vector2){0};

            DrawLine(grid_x, bounds->top, grid_x, bounds->bottom, COL_GRID_VERT);
            (void)snprintf(x_label, sizeof(x_label), "%u", last_tick_idx + 1U);
            label_size = measure_text_font(font, x_label, axis_fs);
            draw_text_font(font,
                       x_label,
                       (float)grid_x - label_size.x * 0.5f,
                       (float)bounds->bottom + LAYOUT_TIMELINE_XLABEL_Y * scale,
                       axis_fs,
                       COL_AXIS_LABEL);
        }

        return;
    }

    {
        char max_tick_label[16] = {0};
        float min_label_spacing = 0.0f;
        Vector2 max_label_size = (Vector2){0};
        unsigned int max_labels = 0U;
        unsigned int x_steps = 0U;
        unsigned int step = 0U;

        (void)snprintf(max_tick_label, sizeof(max_tick_label), "%u", scenario->tick_count);
        max_label_size = measure_text_font(font, max_tick_label, axis_fs);
        min_label_spacing = max_label_size.x + 12.0f * scale;
        if (min_label_spacing < (28.0f * scale)) {
            min_label_spacing = 28.0f * scale;
        }

        max_labels = (unsigned int)((float)bounds->width / min_label_spacing) + 1U;
        if (max_labels < 2U) {
            max_labels = 2U;
        }

        x_steps = (scenario->tick_count <= max_labels) ? (scenario->tick_count - 1U) : (max_labels - 1U);

        for (step = 0U; step <= x_steps; ++step) {
            float t = (x_steps == 0U) ? 0.0f : ((float)step / (float)x_steps);
            unsigned int tick_idx = (unsigned int)roundf(t * (float)(scenario->tick_count - 1U));
            int grid_x = bounds->left + (int)roundf(((float)tick_idx / (float)(scenario->tick_count - 1U)) *
                                (float)bounds->width);
            char x_label[16] = {0};
            Vector2 label_size = (Vector2){0};

            DrawLine(grid_x, bounds->top, grid_x, bounds->bottom, COL_GRID_VERT);
            (void)snprintf(x_label, sizeof(x_label), "%u", tick_idx + 1U);
            label_size = measure_text_font(font, x_label, axis_fs);
            draw_text_font(font,
                       x_label,
                       (float)grid_x - label_size.x * 0.5f,
                       (float)bounds->bottom + LAYOUT_TIMELINE_XLABEL_Y * scale,
                       axis_fs,
                       COL_AXIS_LABEL);
        }
    }
}

static void draw_timeline_thresholds(const TimelinePlotBounds *bounds)
{
    int temp_warn_y = bounds->bottom - (int)((TEMP_WARNING_THRESHOLD / 120.0f) * (float)bounds->height);
    int temp_shutdown_y = bounds->bottom - (int)((TEMP_SHUTDOWN_THRESHOLD / 120.0f) * (float)bounds->height);
    int oil_shutdown_y = bounds->bottom - (int)((OIL_SHUTDOWN_THRESHOLD / 5.0f) * (float)bounds->height);

    draw_dotted_hline(bounds->left, bounds->right, temp_warn_y, 12, 2.0f, COL_WARNING_DASH);
    draw_dotted_hline(bounds->left, bounds->right, temp_shutdown_y, 12, 2.0f, COL_SHUTDOWN_DASH);
    draw_dotted_hline(bounds->left, bounds->right, oil_shutdown_y, 12, 2.0f, COL_OIL_SHUT_DASH);
}

static void draw_timeline_fault_bands(const ScenarioData *scenario, const TimelinePlotBounds *bounds)
{
    unsigned int idx = 0U;

    if (scenario->tick_count <= 1U) {
        return;
    }

    for (idx = 1U; idx < scenario->tick_count; ++idx) {
        SeverityLevel level =
            visualizer_mode_to_level(scenario->ticks[idx].engine_mode, scenario->ticks[idx].result);
        float start_t = (float)(idx - 1U) / (float)(scenario->tick_count - 1U);
        float end_t = (float)idx / (float)(scenario->tick_count - 1U);
        int start_x = bounds->left + (int)(start_t * (float)bounds->width);
        int end_x = bounds->left + (int)(end_t * (float)bounds->width);

        if (level == LEVEL_WARNING) {
            DrawRectangle(start_x, bounds->top, end_x - start_x, bounds->height, COL_WARNING_TINT);
        } else if (level == LEVEL_SHUTDOWN) {
            DrawRectangle(start_x, bounds->top, end_x - start_x, bounds->height, COL_SHUTDOWN_TINT);
        }
    }
}

static void draw_timeline_series(const ScenarioData *scenario, const TimelinePlotBounds *bounds)
{
    unsigned int idx = 0U;

    if (scenario->tick_count <= 1U) {
        return;
    }

    for (idx = 1U; idx < scenario->tick_count; ++idx) {
        float start_t = (float)(idx - 1U) / (float)(scenario->tick_count - 1U);
        float end_t = (float)idx / (float)(scenario->tick_count - 1U);
        int x0 = bounds->left + (int)(start_t * (float)bounds->width);
        int x1 = bounds->left + (int)(end_t * (float)bounds->width);
        float rpm0 = visualizer_clamp01(scenario->ticks[idx - 1U].rpm / 5000.0f);
        float rpm1 = visualizer_clamp01(scenario->ticks[idx].rpm / 5000.0f);
        float temp0 = visualizer_clamp01(scenario->ticks[idx - 1U].temp / 120.0f);
        float temp1 = visualizer_clamp01(scenario->ticks[idx].temp / 120.0f);
        float oil0 = visualizer_clamp01(scenario->ticks[idx - 1U].oil / 5.0f);
        float oil1 = visualizer_clamp01(scenario->ticks[idx].oil / 5.0f);
        float ctrl0 = visualizer_clamp01(scenario->ticks[idx - 1U].control / 100.0f);
        float ctrl1 = visualizer_clamp01(scenario->ticks[idx].control / 100.0f);
        int rpm_y0 = bounds->bottom - (int)(rpm0 * (float)bounds->height);
        int rpm_y1 = bounds->bottom - (int)(rpm1 * (float)bounds->height);
        int temp_y0 = bounds->bottom - (int)(temp0 * (float)bounds->height);
        int temp_y1 = bounds->bottom - (int)(temp1 * (float)bounds->height);
        int oil_y0 = bounds->bottom - (int)(oil0 * (float)bounds->height);
        int oil_y1 = bounds->bottom - (int)(oil1 * (float)bounds->height);
        int ctrl_y0 = bounds->bottom - (int)(ctrl0 * (float)bounds->height);
        int ctrl_y1 = bounds->bottom - (int)(ctrl1 * (float)bounds->height);

        DrawLineEx((Vector2){(float)x0, (float)rpm_y0}, (Vector2){(float)x1, (float)rpm_y1}, 2.0f, COL_RPM);
        DrawLineEx((Vector2){(float)x0, (float)temp_y0}, (Vector2){(float)x1, (float)temp_y1}, 2.0f, COL_TEMP);
        DrawLineEx((Vector2){(float)x0, (float)oil_y0}, (Vector2){(float)x1, (float)oil_y1}, 2.0f, COL_OIL);
        DrawLineEx((Vector2){(float)x0, (float)ctrl_y0}, (Vector2){(float)x1, (float)ctrl_y1}, 2.0f, COL_CTRL);
    }
}

static void draw_timeline_playhead(const TimelinePlotBounds *bounds, const ScenarioData *scenario, float playhead)
{
    float marker_t = 0.0f;
    int marker_x = 0;

    if (scenario->tick_count <= 1U) {
        return;
    }

    marker_t = visualizer_clamp01(playhead / (float)(scenario->tick_count - 1U));
    marker_x = bounds->left + (int)roundf(marker_t * (float)bounds->width);
    DrawLineEx((Vector2){(float)marker_x, (float)bounds->top},
           (Vector2){(float)marker_x, (float)bounds->bottom},
           2.0f,
           COL_PLAYHEAD);
}

static Color timeline_legend_color(int legend_index)
{
    if (legend_index == 1) {
        return COL_OIL;
    }
    if (legend_index == 2) {
        return COL_TEMP;
    }
    if (legend_index == 3) {
        return COL_RPM;
    }

    return COL_CTRL;
}

static void draw_timeline_legend(const Font *font, Rectangle area, float scale)
{
    static const char *const k_legend_names[] = {
        "CTRL",
        "OIL",
        "TEMP",
        "RPM",
    };
    float legend_y = area.y + LAYOUT_TIMELINE_LEGEND_Y * scale;
    float legend_fs = FS_TL_LEGEND * scale;
    float dot_radius = LAYOUT_DOT_R * scale;
    float legend_x = area.x + area.width - LAYOUT_TIMELINE_LEGEND_RIGHT * scale;
    int legend_index = 0;

    for (legend_index = 0; legend_index < 4; ++legend_index) {
        Color legend_color = timeline_legend_color(legend_index);
        Vector2 label_size = measure_text_font(font, k_legend_names[legend_index], legend_fs);

        legend_x -= label_size.x;
        draw_text_font(font, k_legend_names[legend_index], legend_x, legend_y, legend_fs, legend_color);
        legend_x -= dot_radius * 2.0f + LAYOUT_LEGEND_DOT_GAP * scale;
        DrawCircle((int)(legend_x + dot_radius), (int)(legend_y + legend_fs * 0.5f), dot_radius, legend_color);
        legend_x -= LAYOUT_LEGEND_COL_GAP * scale;
    }
}

static void draw_timeline(const Font *font,
              const ScenarioData *scenario,
              float playhead,
              Rectangle area,
              Rectangle plot_area,
              float scale)
{
    TimelinePlotBounds bounds = {0};

    if ((scenario == NULL) || (scenario->tick_count == 0U)) {
        return;
    }

    DrawRectangleRec(area, COL_TIMELINE_BG);
    DrawRectangleLines((int)area.x, (int)area.y, (int)area.width, (int)area.height, COL_TIMELINE_BORDER);
    draw_timeline_heading(font, scenario, playhead, area, scale);
    bounds = timeline_plot_bounds(plot_area);
    draw_timeline_y_axis(font, &bounds, area, scale);
    draw_timeline_x_axis(font, &bounds, scenario, scale);
    draw_timeline_thresholds(&bounds);
    draw_timeline_fault_bands(scenario, &bounds);
    draw_timeline_series(scenario, &bounds);
    draw_timeline_playhead(&bounds, scenario, playhead);
    draw_timeline_legend(font, area, scale);
}

static void draw_slider(
    const Font *font, Rectangle slider, const ScenarioData *scenario, float playhead, float restart_feedback_timer)
{
    float knob_t = 0.0f;
    int knob_x = 0;

    if ((scenario == NULL) || (scenario->tick_count == 0U)) {
        return;
    }

    (void)font;
    (void)restart_feedback_timer;

    {
        int cy = (int)(slider.y + slider.height * 0.5f);
        int th = 6;
        int tx = (int)slider.x;
        int tw = (int)slider.width;
        int rx = 3;

        knob_t = (scenario->tick_count > 1U) ? visualizer_clamp01(playhead / (float)(scenario->tick_count - 1U))
                             : 0.0f;
        knob_x = (int)(slider.x + (slider.width * knob_t));

        DrawRectangle(tx, cy - th / 2, tw, th, COL_SLIDER_TRACK_BG);
        DrawRectangleLines(tx, cy - th / 2, tw, th, COL_SLIDER_TRACK_BOR);
        if (knob_t > 0.0f) {
            int fw = (int)(tw * knob_t);
            DrawRectangle(tx + rx, cy - th / 2, fw - rx, th, COL_SLIDER_FILL);
        }

        DrawCircle(knob_x, cy, LAYOUT_KNOB_SHADOW, COL_KNOB_SHADOW);
        DrawCircle(knob_x, cy, LAYOUT_KNOB_R, COL_KNOB_FILL);
        DrawCircleLines(knob_x, cy, LAYOUT_KNOB_SHADOW, COL_SLIDER_RING);
    }
}

void visualizer_compute_layout(int screen_w, int screen_h, VisualizerLayout *layout)
{
    float content_y = 0.0f;
    float main_h = 0.0f;
    float status_panel_w = 0.0f;
    float col_gap = 0.0f;
    float gauges_x = 0.0f;
    float gauges_w = 0.0f;
    float status_x = 0.0f;
    float gc_x = 0.0f;
    float gc_w = 0.0f;
    float bar_col_w = 0.0f;
    float val_col_w = 0.0f;
    float val_col_x = 0.0f;
    float bar_h = 0.0f;
    float row_step = 0.0f;
    float m_row0_y = 0.0f;
    float m_row1_y = 0.0f;
    float m_row2_y = 0.0f;
    float timeline_y = 0.0f;
    float timeline_h_val = 0.0f;
    float timeline_plot_x = 0.0f;
    float timeline_plot_y = 0.0f;
    float timeline_plot_w = 0.0f;
    float slider_y = 0.0f;
    float slider_h = 0.0f;
    float slider_bottom_pad = 0.0f;
    float xlabel_band = 0.0f;

    if (layout == NULL) {
        return;
    }

    layout->scale = (((float)screen_w / (float)WINDOW_WIDTH) < ((float)screen_h / (float)WINDOW_HEIGHT))
                ? ((float)screen_w / (float)WINDOW_WIDTH)
                : ((float)screen_h / (float)WINDOW_HEIGHT);
    layout->pad = LAYOUT_PAD * layout->scale;
    layout->hdr_h = LAYOUT_HDR_H * layout->scale;
    layout->nav_h = LAYOUT_NAV_H * layout->scale;

    content_y = layout->hdr_h + layout->nav_h + 8.0f * layout->scale;
    main_h = LAYOUT_MAIN_H * layout->scale;
    status_panel_w = LAYOUT_STATUS_W * layout->scale;
    col_gap = LAYOUT_COL_GAP * layout->scale;
    gauges_x = layout->pad;
    gauges_w = (float)screen_w - (layout->pad * 2.0f) - status_panel_w - col_gap;
    status_x = gauges_x + gauges_w + col_gap;
    gc_x = gauges_x + 14.0f * layout->scale;
    gc_w = gauges_w - 28.0f * layout->scale;
    bar_col_w = gc_w * 0.85f;
    val_col_w = gc_w - bar_col_w - 6.0f * layout->scale;
    val_col_x = gc_x + bar_col_w + 6.0f * layout->scale;
    bar_h = LAYOUT_BAR_H * layout->scale;
    row_step = LAYOUT_ROW_STEP * layout->scale;
    m_row0_y = content_y + 60.0f * layout->scale;
    m_row1_y = m_row0_y + row_step;
    m_row2_y = m_row1_y + row_step;

    layout->gauges_panel = (Rectangle){gauges_x, content_y, gauges_w, main_h};
    layout->metrics_area = (Rectangle){status_x, content_y, status_panel_w, main_h};
    layout->rpm_bar = (Rectangle){gc_x, m_row0_y, bar_col_w, bar_h};
    layout->rpm_val = (Rectangle){val_col_x, m_row0_y, val_col_w, bar_h};
    layout->temp_bar = (Rectangle){gc_x, m_row1_y, bar_col_w, bar_h};
    layout->temp_val = (Rectangle){val_col_x, m_row1_y, val_col_w, bar_h};
    layout->oil_bar = (Rectangle){gc_x, m_row2_y, bar_col_w, bar_h};
    layout->oil_val = (Rectangle){val_col_x, m_row2_y, val_col_w, bar_h};

    timeline_y = content_y + main_h + 14.0f * layout->scale;
    timeline_h_val = (float)screen_h - timeline_y - layout->pad;
    layout->timeline = (Rectangle){layout->pad, timeline_y, (float)screen_w - 2.0f * layout->pad, timeline_h_val};

    timeline_plot_x = layout->timeline.x + LAYOUT_TIMELINE_PLOT_LEFT * layout->scale;
    timeline_plot_y = layout->timeline.y + LAYOUT_TIMELINE_PLOT_TOP * layout->scale;
    timeline_plot_w =
        layout->timeline.width - ((LAYOUT_TIMELINE_PLOT_LEFT + LAYOUT_TIMELINE_PLOT_RIGHT) * layout->scale);
    slider_h = LAYOUT_SLIDER_H * layout->scale;
    slider_bottom_pad = LAYOUT_TIMELINE_SLIDER_BOTTOM_PAD * layout->scale;
    xlabel_band = LAYOUT_TIMELINE_XLABEL_BAND * layout->scale;
    slider_y = layout->timeline.y + layout->timeline.height - slider_h - slider_bottom_pad;

    layout->timeline_plot = (Rectangle){
        timeline_plot_x, timeline_plot_y, timeline_plot_w, slider_y - xlabel_band - timeline_plot_y};
    layout->slider = (Rectangle){layout->timeline_plot.x, slider_y, layout->timeline_plot.width, slider_h};
}

static void draw_frame_background(int screen_w, int screen_h, const VisualizerLayout *layout)
{
    ClearBackground(COL_WINDOW_BG);
    DrawRectangleGradientV(0, 0, screen_w, screen_h, COL_GRAD_TOP, COL_GRAD_BOT);
    DrawRectangle(0, 0, screen_w, (int)layout->hdr_h, COL_PANEL_BG);
    DrawLine(0, (int)layout->hdr_h - 1, screen_w, (int)layout->hdr_h - 1, COL_HDR_BORDER);
    DrawRectangle(0, (int)layout->hdr_h, screen_w, (int)layout->nav_h, COL_NAV_BG);
    DrawLine(0,
         (int)(layout->hdr_h + layout->nav_h),
         screen_w,
         (int)(layout->hdr_h + layout->nav_h),
         COL_SECTION_DIV);
}

static void draw_frame_header(const Font *font,
                  int screen_w,
                  const ScenarioSet *scenario_set,
                  const ScenarioData *scenario,
                  const VisualizerLayout *layout,
                  float playhead)
{
    char scen_label[48] = {0};
    char expected_text[64] = {0};
    char tick_str[48] = {0};
    char theme_str[48] = {0};
    unsigned int tick_index = active_tick_index(scenario, playhead);
    Vector2 req_sz = (Vector2){0};
    Vector2 exp_sz = (Vector2){0};
    Vector2 tick_sz = (Vector2){0};
    Vector2 scen_sz = (Vector2){0};
    Vector2 theme_sz = (Vector2){0};
    float header_text_right = 0.0f;
    float scen_fs = 0.0f;
    float meta_fs = FS_TINY * layout->scale;
    float req_x = 0.0f;
    float exp_x = 0.0f;
    float main_row_y = LAYOUT_HEADER_MAIN_Y * layout->scale;
    float meta_row_y = 0.0f;
    float meta_x = 0.0f;
    float badge_w = 0.0f;
    float badge_x = 0.0f;
    float badge_y = 8.0f * layout->scale;
    float theme_badge_w = 0.0f;
    float theme_badge_x = 0.0f;
    float badge_h = LAYOUT_BADGE_H * layout->scale;

    (void)snprintf(scen_label,
               sizeof(scen_label),
               "SCENARIO  %u / %u",
               scenario_set->active_index + 1U,
               scenario_set->count);
    draw_text_font(font,
               scen_label,
               layout->pad,
               LAYOUT_HEADER_TOP_Y * layout->scale,
               FS_TINY * layout->scale,
               COL_SCEN_COUNTER);
    (void)snprintf(tick_str, sizeof(tick_str), "Tick  %u / %u", tick_index + 1U, scenario->tick_count);
    tick_sz = measure_text_font(font, tick_str, FS_BADGE * layout->scale);
    badge_w = tick_sz.x + 20.0f * layout->scale;
    badge_x = (float)screen_w - badge_w - layout->pad;
    (void)snprintf(theme_str, sizeof(theme_str), "THEME  %s", visualizer_theme_label(visualizer_theme_get()));
    theme_sz = measure_text_font(font, theme_str, FS_BADGE * layout->scale);
    theme_badge_w = theme_sz.x + 20.0f * layout->scale;
    theme_badge_x = badge_x - theme_badge_w - 10.0f * layout->scale;
    header_text_right = theme_badge_x - 20.0f * layout->scale;
    scen_fs = pick_header_name_font_size(font,
                         scenario->scenario,
                         scenario->requirement_id,
                         scenario->expected,
                         header_text_right - layout->pad,
                         layout->scale);
    meta_row_y = main_row_y + (scen_fs - meta_fs);
    draw_text_font(font, scenario->scenario, layout->pad, main_row_y, scen_fs, COL_PRIMARY_TEXT);
    scen_sz = measure_text_font(font, scenario->scenario, scen_fs);
    req_x = layout->pad + scen_sz.x + LAYOUT_HEADER_SEGMENT_GAP * layout->scale;
    exp_x = req_x;
    req_sz = measure_text_font(font, scenario->requirement_id, meta_fs);
    if ((req_x + req_sz.x) < header_text_right) {
        draw_text_font(font, scenario->requirement_id, req_x, meta_row_y, meta_fs, COL_SCEN_COUNTER);
        exp_x = req_x + req_sz.x + LAYOUT_HEADER_META_GAP * layout->scale;
    }
    (void)snprintf(expected_text, sizeof(expected_text), "EXPECTED: %s", scenario->expected);
    exp_sz = measure_text_font(font, expected_text, meta_fs);
    meta_x = exp_x;
    if ((meta_x + exp_sz.x) < header_text_right) {
        draw_text_font(font, expected_text, meta_x, meta_row_y, meta_fs, COL_SCEN_COUNTER);
    }
    DrawRectangle((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h, COL_BADGE_BG);
    DrawRectangleLines((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h, COL_BADGE_BORDER);
    draw_text_font(font,
               tick_str,
               badge_x + 10.0f * layout->scale,
               badge_y + (badge_h - FS_BADGE * layout->scale) * 0.5f,
               FS_BADGE * layout->scale,
               COL_BADGE_TEXT);
    if (theme_badge_x <= (meta_x + 24.0f * layout->scale)) {
        return;
    }
    DrawRectangle((int)theme_badge_x, (int)badge_y, (int)theme_badge_w, (int)badge_h, COL_BADGE_BG);
    DrawRectangleLines((int)theme_badge_x, (int)badge_y, (int)theme_badge_w, (int)badge_h, COL_BADGE_BORDER);
    draw_text_font(font,
               theme_str,
               theme_badge_x + 10.0f * layout->scale,
               badge_y + (badge_h - FS_BADGE * layout->scale) * 0.5f,
               FS_BADGE * layout->scale,
               COL_BADGE_TEXT);
}

static void
draw_frame_nav(const Font *font, int screen_w, const VisualizerLayout *layout, float ticks_per_second, int paused)
{
    float key_x = layout->pad;
    float key_y = layout->hdr_h + (layout->nav_h - FS_KEY_HINT * layout->scale) * 0.5f;
    float key_fs = FS_KEY_HINT * layout->scale;
    float key_gap = 20.0f * layout->scale;
    char speed_str[32] = {0};
    Vector2 speed_size = (Vector2){0};

    key_x += draw_key_hint(font, "SPC", "Pause", key_x, key_y, key_fs) + key_gap;
    key_x += draw_key_hint(font, "R", "Restart", key_x, key_y, key_fs) + key_gap;
    key_x += draw_key_hint(font, "UP DN", "Speed", key_x, key_y, key_fs) + key_gap;
    key_x += draw_key_hint(font, "LT RT", "Step", key_x, key_y, key_fs) + key_gap;
    key_x += draw_key_hint(font, "TAB", "Switch", key_x, key_y, key_fs) + key_gap;
    key_x += draw_key_hint(font, "T", "Theme", key_x, key_y, key_fs) + key_gap;
    key_x += draw_key_hint(font, "F11", "Fullscreen", key_x, key_y, key_fs) + key_gap;
    key_x += draw_key_hint(font, "ESC", "Quit", key_x, key_y, key_fs);
    (void)key_x;
    (void)snprintf(speed_str, sizeof(speed_str), "%.0f tk/s%s", ticks_per_second, (paused != 0) ? "  PAUSED" : "");
    speed_size = measure_text_font(font, speed_str, key_fs);
    draw_text_font(font,
               speed_str,
               (float)screen_w - speed_size.x - layout->pad,
               key_y,
               key_fs,
               (paused != 0) ? COL_SPEED_PAUSED : COL_SPEED_RUNNING);
}

static void draw_sensor_panel(const Font *font, const TickData *tick, const VisualizerLayout *layout)
{
    draw_panel_header(font, layout->gauges_panel, "SENSOR READINGS", layout->scale);
    draw_meter(font,
           "RPM",
           tick->rpm,
           5000.0f,
           layout->rpm_bar,
           layout->rpm_val,
           visualizer_rpm_to_level(tick->rpm, tick->temp),
           "rpm",
           RPM_WARNING_THRESHOLD,
           0.0f,
           layout->scale);
    draw_meter(font,
           "Temperature",
           tick->temp,
           120.0f,
           layout->temp_bar,
           layout->temp_val,
           visualizer_temp_to_level(tick->temp),
           "C",
           TEMP_WARNING_THRESHOLD,
           TEMP_SHUTDOWN_THRESHOLD,
           layout->scale);
    draw_meter(font,
           "Oil Pressure",
           tick->oil,
           5.0f,
           layout->oil_bar,
           layout->oil_val,
           visualizer_oil_to_level(tick->oil),
           "bar",
           0.0f,
           OIL_SHUTDOWN_THRESHOLD,
           layout->scale);
}

static void
draw_status_summary(const Font *font, const TickData *tick, const VisualizerLayout *layout, Color animated_mode_color)
{
    float mode_x = layout->metrics_area.x + LAYOUT_STATUS_MODE_X * layout->scale;
    float result_x = layout->metrics_area.x + layout->metrics_area.width * LAYOUT_STATUS_RESULT_X_PCT;
    float run_x = layout->metrics_area.x + layout->metrics_area.width - LAYOUT_STATUS_RUN_RIGHT * layout->scale;
    float caption_y = layout->metrics_area.y + LAYOUT_STATUS_CAP_Y * layout->scale;
    float caption_fs = FS_TINY * layout->scale;
    float value_fs = FS_VALUE * layout->scale;
    float result_y = caption_y + LAYOUT_STATUS_VALUE_GAP * layout->scale;
    char run_text[16] = {0};
    Color summary_color = result_color(tick->result);

    draw_text_font(font, "MODE", mode_x, caption_y, caption_fs, COL_CAPTION);
    draw_text_font(font,
               tick->engine_mode,
               mode_x,
               caption_y + caption_fs + 4.0f * layout->scale,
               value_fs,
               animated_mode_color);
    draw_text_font(font, "RESULT", result_x, caption_y, caption_fs, COL_CAPTION);
    draw_text_font(font, "RUN", run_x, caption_y, caption_fs, COL_CAPTION);
    draw_text_font(font, tick->result, result_x, result_y, value_fs, summary_color);
    (void)snprintf(run_text, sizeof(run_text), "%d", tick->run);
    draw_text_font(font, run_text, run_x, result_y, value_fs, summary_color);
}

static void
draw_status_fault_rates(const Font *font, const VisualizerLayout *layout, float warning_pct, float shutdown_pct)
{
    float base_x = layout->metrics_area.x + 14.0f * layout->scale;
    float base_w = layout->metrics_area.width - 28.0f * layout->scale;
    float bar_h = 9.0f * layout->scale;
    float label_w = 68.0f * layout->scale;
    float track_x = base_x + label_w + LAYOUT_STATUS_FAULT_LABEL_GAP * layout->scale;
    float track_w = base_w - label_w - LAYOUT_STATUS_FAULT_LABEL_GAP * layout->scale - 52.0f * layout->scale;

    DrawLine((int)(layout->metrics_area.x + 14.0f * layout->scale),
         (int)(layout->metrics_area.y + LAYOUT_STATUS_FAULT_DIV_Y * layout->scale),
         (int)(layout->metrics_area.x + layout->metrics_area.width - 14.0f * layout->scale),
         (int)(layout->metrics_area.y + LAYOUT_STATUS_FAULT_DIV_Y * layout->scale),
         COL_SECTION_DIV);
    draw_text_font(font,
               "SESSION FAULT RATE",
               layout->metrics_area.x + 14.0f * layout->scale,
               layout->metrics_area.y + LAYOUT_STATUS_FAULT_TITLE_Y * layout->scale,
               FS_TINY * layout->scale,
               COL_CAPTION);
    draw_fault_rate_row(font,
                "WARNING",
                warning_pct,
                base_x,
                track_x,
                track_w,
                bar_h,
                layout->metrics_area.y + LAYOUT_STATUS_FAULT_ROW0_Y * layout->scale,
                COL_WARNING_FILL,
                layout->scale);
    draw_fault_rate_row(font,
                "SHUTDOWN",
                shutdown_pct,
                base_x,
                track_x,
                track_w,
                bar_h,
                layout->metrics_area.y + LAYOUT_STATUS_FAULT_ROW1_Y * layout->scale,
                COL_SHUTDOWN_FILL,
                layout->scale);
}

static void
draw_status_end_notice(const Font *font, const VisualizerLayout *layout, const ScenarioData *scenario, float playhead)
{
    if (playhead < (float)(scenario->tick_count - 1U)) {
        return;
    }

    DrawLine((int)(layout->metrics_area.x + 14.0f * layout->scale),
         (int)(layout->metrics_area.y + LAYOUT_STATUS_END_DIV_Y * layout->scale),
         (int)(layout->metrics_area.x + layout->metrics_area.width - 14.0f * layout->scale),
         (int)(layout->metrics_area.y + LAYOUT_STATUS_END_DIV_Y * layout->scale),
         COL_SECTION_DIV);
    draw_text_font(font,
               "End of scenario",
               layout->metrics_area.x + 14.0f * layout->scale,
               layout->metrics_area.y + LAYOUT_STATUS_END_TITLE_Y * layout->scale,
               FS_SMALL * layout->scale,
               COL_END_NOTICE);
    draw_text_font(font,
               "Press  R  to replay",
               layout->metrics_area.x + 14.0f * layout->scale,
               layout->metrics_area.y + LAYOUT_STATUS_END_HINT_Y * layout->scale,
               FS_SMALL * layout->scale,
               COL_REPLAY_HINT);
}

static void draw_status_panel(const Font *font,
                  const ScenarioData *scenario,
                  const TickData *tick,
                  const VisualizerLayout *layout,
                  float playhead,
                  Color animated_mode_color,
                  float warning_pct,
                  float shutdown_pct)
{
    draw_panel_header(font, layout->metrics_area, "ENGINE STATUS", layout->scale);
    draw_status_summary(font, tick, layout, animated_mode_color);
    draw_status_fault_rates(font, layout, warning_pct, shutdown_pct);
    draw_status_end_notice(font, layout, scenario, playhead);
}

void visualizer_draw_frame(const Font *font,
               const ScenarioSet *scenario_set,
               const ScenarioData *scenario,
               const TickData *tick,
               const VisualizerLayout *layout,
               float playhead,
               float ticks_per_second,
               int paused,
               Color animated_mode_color,
               float restart_feedback_timer,
               float warning_pct,
               float shutdown_pct,
               int quit_modal_open,
               int quit_modal_selection)
{
    int screen_w = 0;
    int screen_h = 0;

    if ((font == NULL) || (scenario_set == NULL) || (scenario == NULL) || (tick == NULL) || (layout == NULL)) {
        return;
    }

    screen_w = GetScreenWidth();
    screen_h = GetScreenHeight();

    BeginDrawing();
    draw_frame_background(screen_w, screen_h, layout);
    draw_frame_header(font, screen_w, scenario_set, scenario, layout, playhead);
    draw_frame_nav(font, screen_w, layout, ticks_per_second, paused);
    draw_sensor_panel(font, tick, layout);
    draw_status_panel(font, scenario, tick, layout, playhead, animated_mode_color, warning_pct, shutdown_pct);
    draw_timeline(font, scenario, playhead, layout->timeline, layout->timeline_plot, layout->scale);
    draw_slider(font, layout->slider, scenario, playhead, restart_feedback_timer);
    if (quit_modal_open != 0) {
        draw_quit_modal(font, quit_modal_selection, screen_w, screen_h, layout->scale);
    }

    EndDrawing();
}
