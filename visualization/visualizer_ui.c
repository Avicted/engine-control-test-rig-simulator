#include <math.h>
#include <stdio.h>
#include <string.h>

#include "visualizer_playback.h"
#include "visualizer_ui.h"

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

#define FS_TINY 11.0f
#define FS_SMALL 12.0f
#define FS_KEY_HINT 13.0f
#define FS_VALUE 16.0f
#define FS_BADGE 15.0f
#define FS_MODE 22.0f
#define FS_SCEN_NAME 20.0f
#define FS_TL_TITLE 18.0f
#define FS_TL_LEGEND 17.0f
#define FS_TL_AXIS 14.0f

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
#define LAYOUT_DOT_R 5.0f
#define LAYOUT_LEGEND_DOT_GAP 6.0f
#define LAYOUT_LEGEND_COL_GAP 18.0f
#define LAYOUT_SLIDER_INSET 30.0f
#define LAYOUT_SLIDER_H 24.0f

#define COL_REPLAY_HINT ((Color){160, 174, 210, 255})

static void draw_text_font(const Font *font, const char *text, float x, float y, float size, Color color)
{
    if ((font == NULL) || (text == NULL))
    {
        return;
    }
    DrawTextEx(*font, text, (Vector2){x, y}, size, 1.0f, color);
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

Color visualizer_mode_color(const char *engine_mode)
{
    if (engine_mode == NULL)
    {
        return COL_MODE_DEFAULT;
    }
    if ((strcmp(engine_mode, "INIT") == 0) || (strcmp(engine_mode, "STARTING") == 0))
    {
        return COL_RPM;
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

Color visualizer_lerp_color(Color a, Color b, float t)
{
    Color out;
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
    Vector2 sz;
    Color yes_bg;
    Color yes_bor;
    Color yes_txt;
    Color no_bg;
    Color no_bor;
    Color no_txt;

    DrawRectangle(0, 0, screen_w, screen_h, (Color){0, 0, 0, 160});
    DrawRectangle((int)(box_x + 4.0f), (int)(box_y + 4.0f), (int)box_w, (int)box_h, (Color){0, 0, 0, 110});
    DrawRectangle((int)box_x, (int)box_y, (int)box_w, (int)box_h, COL_PANEL_BG);
    DrawRectangleLines((int)box_x, (int)box_y, (int)box_w, (int)box_h, COL_PANEL_BORDER);

    sz = MeasureTextEx(*font, title, title_fs, 1.0f);
    draw_text_font(font, title,
                   box_x + (box_w - sz.x) * 0.5f,
                   box_y + 14.0f * scale,
                   title_fs, RAYWHITE);

    DrawLine((int)(box_x + 16.0f * scale), (int)(box_y + 54.0f * scale),
             (int)(box_x + box_w - 16.0f * scale), (int)(box_y + 54.0f * scale),
             COL_SECTION_DIV);

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
    float cx = x;
    float w;

    w = MeasureTextEx(*font, "[", font_size, 1.0f).x;
    draw_text_font(font, "[", cx, y, font_size, COL_BRAK);
    cx += w;
    w = MeasureTextEx(*font, key, font_size, 1.0f).x;
    draw_text_font(font, key, cx, y, font_size, COL_KEY);
    cx += w;
    w = MeasureTextEx(*font, "]", font_size, 1.0f).x;
    draw_text_font(font, "]", cx, y, font_size, COL_BRAK);
    cx += w + 4.0f;
    w = MeasureTextEx(*font, desc, font_size, 1.0f).x;
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
    float ratio;
    int fill_width;
    char value_text[64];
    float lbl_fs = FS_KEY_HINT * scale;
    float val_fs = FS_VALUE * scale;
    Vector2 value_size;
    float value_x;
    float value_y;

    ratio = (max_value > 0.0f) ? (value / max_value) : 0.0f;
    ratio = visualizer_clamp01(ratio);
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
            SeverityLevel level = visualizer_mode_to_level(scenario->ticks[idx].engine_mode, scenario->ticks[idx].result);
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

            float rpm0 = visualizer_clamp01(scenario->ticks[idx - 1U].rpm / 5000.0f);
            float rpm1 = visualizer_clamp01(scenario->ticks[idx].rpm / 5000.0f);
            float temp0 = visualizer_clamp01(scenario->ticks[idx - 1U].temp / 120.0f);
            float temp1 = visualizer_clamp01(scenario->ticks[idx].temp / 120.0f);
            float oil0 = visualizer_clamp01(scenario->ticks[idx - 1U].oil / 5.0f);
            float oil1 = visualizer_clamp01(scenario->ticks[idx].oil / 5.0f);
            float ctrl0 = visualizer_clamp01(scenario->ticks[idx - 1U].control / 100.0f);
            float ctrl1 = visualizer_clamp01(scenario->ticks[idx].control / 100.0f);

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
            float marker_t = visualizer_clamp01(playhead / (float)(scenario->tick_count - 1U));
            int marker_x = plot_left + (int)(marker_t * (float)plot_w);
            DrawLineEx((Vector2){(float)marker_x, (float)plot_top},
                       (Vector2){(float)marker_x, (float)plot_bottom},
                       2.0f, COL_PLAYHEAD);
        }
    }

    {
        float ley = area.y + 8.0f;
        float lfs2 = FS_TL_LEGEND;
        float dot_r = LAYOUT_DOT_R;
        float lx = area.x + area.width - 12.0f;
        Vector2 lsz;
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
    (void)restart_feedback_timer;

    {
        int cy = (int)(slider.y + slider.height * 0.5f);
        int th = 6;
        int tx = (int)slider.x;
        int tw = (int)slider.width;
        int rx = 3;

        knob_t = (scenario->tick_count > 1U) ? visualizer_clamp01(playhead / (float)(scenario->tick_count - 1U)) : 0.0f;
        knob_x = (int)(slider.x + (slider.width * knob_t));

        DrawRectangle(tx, cy - th / 2, tw, th, COL_SLIDER_TRACK_BG);
        DrawRectangleLines(tx, cy - th / 2, tw, th, COL_SLIDER_TRACK_BOR);
        if (knob_t > 0.0f)
        {
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
    float content_y;
    float main_h;
    float status_panel_w;
    float col_gap;
    float gauges_x;
    float gauges_w;
    float status_x;
    float gc_x;
    float gc_w;
    float bar_col_w;
    float val_col_w;
    float val_col_x;
    float bar_h;
    float row_step;
    float m_row0_y;
    float m_row1_y;
    float m_row2_y;
    float timeline_y;
    float timeline_h_val;

    if (layout == NULL)
    {
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

    timeline_y = content_y + main_h + 10.0f * layout->scale;
    timeline_h_val = (float)screen_h - timeline_y - layout->pad;
    layout->timeline = (Rectangle){layout->pad, timeline_y, (float)screen_w - 2.0f * layout->pad, timeline_h_val};
    layout->slider = (Rectangle){layout->timeline.x + 52.0f,
                                 layout->timeline.y + layout->timeline.height - LAYOUT_SLIDER_INSET * layout->scale,
                                 layout->timeline.width - 76.0f,
                                 LAYOUT_SLIDER_H * layout->scale};
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
    int screen_w;
    int screen_h;

    if ((font == NULL) || (scenario_set == NULL) || (scenario == NULL) || (tick == NULL) || (layout == NULL))
    {
        return;
    }

    screen_w = GetScreenWidth();
    screen_h = GetScreenHeight();

    BeginDrawing();
    ClearBackground(COL_WINDOW_BG);
    DrawRectangleGradientV(0, 0, screen_w, screen_h, COL_GRAD_TOP, COL_GRAD_BOT);

    DrawRectangle(0, 0, screen_w, (int)layout->hdr_h, COL_PANEL_BG);
    DrawLine(0, (int)layout->hdr_h - 1, screen_w, (int)layout->hdr_h - 1, COL_HDR_BORDER);
    {
        char scen_label[48];
        char meta_text[96];
        char tick_str[48];
        Vector2 tick_sz;
        Vector2 scen_sz;
        float meta_x;
        float meta_y;
        float badge_w;
        float badge_x;
        float badge_y;
        float badge_h = LAYOUT_BADGE_H * layout->scale;

        (void)snprintf(scen_label, sizeof(scen_label),
                       "SCENARIO  %u / %u",
                       scenario_set->active_index + 1U, scenario_set->count);
        draw_text_font(font, scen_label, layout->pad, 7.0f * layout->scale, FS_TINY * layout->scale, COL_SCEN_COUNTER);
        (void)snprintf(tick_str, sizeof(tick_str), "Tick  %u / %u",
                       tick->tick, scenario->tick_count);
        tick_sz = MeasureTextEx(*font, tick_str, FS_BADGE * layout->scale, 1.0f);
        badge_w = tick_sz.x + 20.0f * layout->scale;
        badge_x = (float)screen_w - badge_w - layout->pad;
        badge_y = (layout->hdr_h - badge_h) * 0.5f;

        draw_text_font(font, scenario->scenario, layout->pad, 21.0f * layout->scale, FS_SCEN_NAME * layout->scale, RAYWHITE);
        scen_sz = MeasureTextEx(*font, scenario->scenario, FS_SCEN_NAME * layout->scale, 1.0f);
        (void)snprintf(meta_text, sizeof(meta_text), "%s   EXPECTED: %s",
                       scenario->requirement_id, scenario->expected);
        meta_x = layout->pad + scen_sz.x + 32.0f * layout->scale;
        meta_y = 18.0f * layout->scale + (FS_SCEN_NAME - FS_KEY_HINT) * 0.5f * layout->scale;
        if (meta_x < (badge_x - 220.0f * layout->scale))
        {
            draw_text_font(font, meta_text, meta_x, meta_y, FS_SCEN_NAME * layout->scale, COL_SCEN_COUNTER);
        }

        DrawRectangle((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h, COL_BADGE_BG);
        DrawRectangleLines((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h, COL_BADGE_BORDER);
        draw_text_font(font, tick_str,
                       badge_x + 10.0f * layout->scale,
                       badge_y + (badge_h - FS_BADGE * layout->scale) * 0.5f,
                       FS_BADGE * layout->scale, COL_BADGE_TEXT);
    }

    DrawRectangle(0, (int)layout->hdr_h, screen_w, (int)layout->nav_h, COL_NAV_BG);
    DrawLine(0, (int)(layout->hdr_h + layout->nav_h), screen_w, (int)(layout->hdr_h + layout->nav_h), COL_SECTION_DIV);
    {
        float kx = layout->pad;
        float ky = layout->hdr_h + (layout->nav_h - FS_KEY_HINT * layout->scale) * 0.5f;
        float kfs = FS_KEY_HINT * layout->scale;
        float gap2 = 20.0f * layout->scale;
        char speed_str[32];
        Vector2 sp_sz;

        kx += draw_key_hint(font, "SPC", "Pause", kx, ky, kfs) + gap2;
        kx += draw_key_hint(font, "R", "Restart", kx, ky, kfs) + gap2;
        kx += draw_key_hint(font, "UP DN", "Speed", kx, ky, kfs) + gap2;
        kx += draw_key_hint(font, "LT RT", "Step", kx, ky, kfs) + gap2;
        kx += draw_key_hint(font, "TAB", "Switch", kx, ky, kfs) + gap2;
        kx += draw_key_hint(font, "F11", "Fullscreen", kx, ky, kfs) + gap2;
        kx += draw_key_hint(font, "ESC", "Quit", kx, ky, kfs);
        (void)kx;

        (void)snprintf(speed_str, sizeof(speed_str), "%.0f tk/s%s",
                       ticks_per_second, (paused != 0) ? "  PAUSED" : "");
        sp_sz = MeasureTextEx(*font, speed_str, kfs, 1.0f);
        draw_text_font(font, speed_str,
                       (float)screen_w - sp_sz.x - layout->pad, ky, kfs,
                       (paused != 0) ? COL_SPEED_PAUSED : COL_SPEED_RUNNING);
    }

    draw_panel_header(font, layout->gauges_panel, "SENSOR READINGS", layout->scale);

    draw_meter(font, "RPM", tick->rpm, 5000.0f,
               layout->rpm_bar, layout->rpm_val,
               visualizer_rpm_to_level(tick->rpm, tick->temp),
               "rpm", RPM_WARNING_THRESHOLD, 0.0f, layout->scale);
    draw_meter(font, "Temperature", tick->temp, 120.0f,
               layout->temp_bar, layout->temp_val,
               visualizer_temp_to_level(tick->temp),
               "C", TEMP_WARNING_THRESHOLD, TEMP_SHUTDOWN_THRESHOLD, layout->scale);
    draw_meter(font, "Oil Pressure", tick->oil, 5.0f,
               layout->oil_bar, layout->oil_val,
               visualizer_oil_to_level(tick->oil),
               "bar", 0.0f, OIL_SHUTDOWN_THRESHOLD, layout->scale);
    draw_panel_header(font, layout->metrics_area, "ENGINE STATUS", layout->scale);

    {
        float lx = layout->metrics_area.x + 14.0f * layout->scale;
        float right_cols_x0 = layout->metrics_area.x + layout->metrics_area.width * 0.45f;
        float right_cols_x1 = layout->metrics_area.x + layout->metrics_area.width - 8.0f * layout->scale;
        float right_col_w = (right_cols_x1 - right_cols_x0) * 0.8f;
        float rx = right_cols_x0;
        float run_col_x = right_cols_x0 + right_col_w;
        float cap_y = layout->metrics_area.y + 38.0f * layout->scale;
        float cap_fs = FS_TINY * layout->scale;
        float mode_fs = FS_MODE * layout->scale;
        char run_text[16];
        float result_y;
        Color result_c;

        result_c = (strcmp(tick->result, "OK") == 0)
                       ? COL_OIL
                   : (strcmp(tick->result, "WARNING") == 0)
                       ? COL_WARNING
                       : COL_SHUTDOWN;

        draw_text_font(font, "MODE", lx, cap_y, cap_fs, COL_CAPTION);
        draw_text_font(font, tick->engine_mode, lx, cap_y + cap_fs + 4.0f * layout->scale,
                       mode_fs, animated_mode_color);

        draw_text_font(font, "RESULT", rx, cap_y, cap_fs, COL_CAPTION);
        draw_text_font(font, "RUN", run_col_x, cap_y, cap_fs, COL_CAPTION);
        result_y = cap_y + cap_fs + 4.0f * layout->scale;
        draw_text_font(font, tick->result, rx, result_y, mode_fs, result_c);

        (void)snprintf(run_text, sizeof(run_text), "%d", tick->run);
        draw_text_font(font, run_text, run_col_x, result_y, mode_fs, result_c);
    }

    DrawLine((int)(layout->metrics_area.x + 14.0f * layout->scale), (int)(layout->metrics_area.y + 110.0f * layout->scale),
             (int)(layout->metrics_area.x + layout->metrics_area.width - 14.0f * layout->scale),
             (int)(layout->metrics_area.y + 110.0f * layout->scale),
             COL_SECTION_DIV);

    draw_text_font(font, "SESSION FAULT RATE",
                   layout->metrics_area.x + 14.0f * layout->scale, layout->metrics_area.y + 118.0f * layout->scale,
                   FS_TINY * layout->scale, COL_CAPTION);
    {
        float bx2 = layout->metrics_area.x + 14.0f * layout->scale;
        float bw2 = layout->metrics_area.width - 28.0f * layout->scale;
        float bh2 = 9.0f * layout->scale;
        float lbl_w2 = 68.0f * layout->scale;
        float tr_x = bx2 + lbl_w2;
        float tr_w = bw2 - lbl_w2 - 52.0f * layout->scale;

        draw_fault_rate_row(font, "WARNING", warning_pct,
                            bx2, tr_x, tr_w, bh2,
                            layout->metrics_area.y + 134.0f * layout->scale,
                            COL_WARNING_FILL, layout->scale);
        draw_fault_rate_row(font, "SHUTDOWN", shutdown_pct,
                            bx2, tr_x, tr_w, bh2,
                            layout->metrics_area.y + 148.0f * layout->scale,
                            COL_SHUTDOWN_FILL, layout->scale);
    }

    if (playhead >= (float)(scenario->tick_count - 1U))
    {
        DrawLine((int)(layout->metrics_area.x + 14.0f * layout->scale), (int)(layout->metrics_area.y + 172.0f * layout->scale),
                 (int)(layout->metrics_area.x + layout->metrics_area.width - 14.0f * layout->scale),
                 (int)(layout->metrics_area.y + 172.0f * layout->scale),
                 COL_SECTION_DIV);
        draw_text_font(font, "End of scenario",
                       layout->metrics_area.x + 14.0f * layout->scale, layout->metrics_area.y + 180.0f * layout->scale,
                       FS_SMALL * layout->scale, COL_END_NOTICE);
        draw_text_font(font, "Press  R  to replay",
                       layout->metrics_area.x + 14.0f * layout->scale, layout->metrics_area.y + 194.0f * layout->scale,
                       FS_SMALL * layout->scale, COL_REPLAY_HINT);
    }

    draw_timeline(font, scenario, playhead, layout->timeline, layout->scale);
    draw_slider(font, layout->slider, scenario, playhead, restart_feedback_timer);

    if (quit_modal_open != 0)
    {
        draw_quit_modal(font, quit_modal_selection, screen_w, screen_h, layout->scale);
    }

    EndDrawing();
}