#include <math.h>
#include <stdio.h>
#include <string.h>

#include "visualizer_playback.h"
#include "visualizer_ui.h"

typedef struct
{
    const char *label;
    Color window_bg;
    Color grad_top;
    Color grad_bot;
    Color panel_bg;
    Color nav_bg;
    Color timeline_bg;
    Color bar_bg;
    Color badge_bg;
    Color slider_track_bg;
    Color knob_shadow;
    Color hdr_border;
    Color panel_border;
    Color timeline_border;
    Color section_div;
    Color timeline_subdiv;
    Color fault_bar_bor;
    Color bar_border;
    Color slider_track_bor;
    Color badge_border;
    Color grid_line;
    Color grid_bright;
    Color grid_vert;
    Color caption;
    Color timeline_title;
    Color speed_running;
    Color scen_counter;
    Color sublabel;
    Color key;
    Color key_desc;
    Color meter_label;
    Color tick_counter;
    Color badge_text;
    Color axis_label;
    Color fault_pct_text;
    Color primary_text;
    Color knob_fill;
    Color mode_default;
    Color brak;
    Color ok;
    Color warning;
    Color shutdown;
    Color warning_fill;
    Color shutdown_fill;
    Color warning_dash;
    Color shutdown_dash;
    Color oil_shut_dash;
    Color warning_tint;
    Color shutdown_tint;
    Color speed_paused;
    Color end_notice;
    Color playhead;
    Color rpm;
    Color temp;
    Color oil;
    Color ctrl;
    Color slider_fill;
    Color slider_ring;
    Color replay_hint;
} VisualizerTheme;

static const VisualizerTheme k_visualizer_themes[VISUALIZER_THEME_COUNT] = {
    [VISUALIZER_THEME_MIDNIGHT] = {
        .label = "MIDNIGHT",
        .window_bg = (Color){9, 11, 18, 255},
        .grad_top = (Color){11, 14, 22, 255},
        .grad_bot = (Color){7, 9, 16, 255},
        .panel_bg = (Color){13, 16, 26, 255},
        .nav_bg = (Color){10, 12, 20, 255},
        .timeline_bg = (Color){12, 15, 24, 255},
        .bar_bg = (Color){18, 22, 34, 255},
        .badge_bg = (Color){20, 25, 40, 255},
        .slider_track_bg = (Color){22, 27, 44, 255},
        .knob_shadow = (Color){8, 10, 18, 180},
        .hdr_border = (Color){30, 36, 58, 255},
        .panel_border = (Color){30, 36, 56, 255},
        .timeline_border = (Color){32, 38, 58, 255},
        .section_div = (Color){24, 29, 48, 255},
        .timeline_subdiv = (Color){26, 31, 50, 255},
        .fault_bar_bor = (Color){38, 44, 64, 255},
        .bar_border = (Color){40, 46, 66, 255},
        .slider_track_bor = (Color){44, 52, 76, 255},
        .badge_border = (Color){44, 54, 84, 255},
        .grid_line = (Color){44, 49, 63, 180},
        .grid_bright = (Color){96, 104, 122, 220},
        .grid_vert = (Color){38, 43, 58, 170},
        .caption = (Color){68, 82, 118, 255},
        .timeline_title = (Color){72, 85, 120, 255},
        .speed_running = (Color){72, 86, 120, 255},
        .scen_counter = (Color){78, 94, 136, 255},
        .sublabel = (Color){90, 106, 148, 255},
        .key = (Color){90, 170, 255, 255},
        .key_desc = (Color){110, 124, 155, 255},
        .meter_label = (Color){120, 134, 165, 255},
        .tick_counter = (Color){140, 155, 186, 255},
        .badge_text = (Color){148, 165, 205, 255},
        .axis_label = (Color){160, 168, 184, 255},
        .fault_pct_text = (Color){170, 182, 212, 255},
        .primary_text = (Color){245, 248, 255, 255},
        .knob_fill = (Color){220, 228, 245, 255},
        .mode_default = (Color){180, 186, 200, 255},
        .brak = (Color){50, 62, 94, 255},
        .ok = (Color){40, 167, 69, 255},
        .warning = (Color){255, 193, 7, 255},
        .shutdown = (Color){220, 53, 69, 255},
        .warning_fill = (Color){255, 193, 7, 200},
        .shutdown_fill = (Color){220, 53, 69, 210},
        .warning_dash = (Color){255, 193, 7, 160},
        .shutdown_dash = (Color){220, 53, 69, 170},
        .oil_shut_dash = (Color){220, 53, 69, 120},
        .warning_tint = (Color){255, 193, 7, 26},
        .shutdown_tint = (Color){220, 53, 69, 34},
        .speed_paused = (Color){255, 193, 7, 220},
        .end_notice = (Color){255, 220, 100, 255},
        .playhead = (Color){255, 255, 255, 210},
        .rpm = (Color){52, 152, 219, 255},
        .temp = (Color){255, 99, 132, 255},
        .oil = (Color){46, 204, 113, 255},
        .ctrl = (Color){176, 132, 255, 255},
        .slider_fill = (Color){48, 128, 220, 255},
        .slider_ring = (Color){48, 128, 220, 200},
        .replay_hint = (Color){160, 174, 210, 255},
    },
    [VISUALIZER_THEME_DOS_BLUE] = {
        .label = "DOS BLUE",
        .window_bg = (Color){0, 0, 170, 255},
        .grad_top = (Color){0, 0, 184, 255},
        .grad_bot = (Color){0, 0, 136, 255},
        .panel_bg = (Color){0, 0, 164, 255},
        .nav_bg = (Color){0, 0, 144, 255},
        .timeline_bg = (Color){0, 0, 152, 255},
        .bar_bg = (Color){0, 0, 112, 255},
        .badge_bg = (Color){0, 0, 136, 255},
        .slider_track_bg = (Color){0, 0, 104, 255},
        .knob_shadow = (Color){0, 0, 0, 120},
        .hdr_border = (Color){60, 120, 220, 255},
        .panel_border = (Color){60, 120, 220, 255},
        .timeline_border = (Color){60, 120, 220, 255},
        .section_div = (Color){50, 100, 200, 255},
        .timeline_subdiv = (Color){70, 140, 220, 255},
        .fault_bar_bor = (Color){60, 100, 180, 255},
        .bar_border = (Color){60, 100, 180, 255},
        .slider_track_bor = (Color){90, 170, 220, 255},
        .badge_border = (Color){90, 170, 220, 255},
        .grid_line = (Color){70, 100, 180, 70},
        .grid_bright = (Color){150, 200, 255, 120},
        .grid_vert = (Color){60, 110, 200, 70},
        .caption = (Color){150, 220, 255, 255},
        .timeline_title = (Color){180, 230, 255, 255},
        .speed_running = (Color){180, 230, 255, 255},
        .scen_counter = (Color){160, 200, 240, 255},
        .sublabel = (Color){145, 185, 225, 255},
        .key = (Color){255, 255, 255, 255},
        .key_desc = (Color){170, 205, 240, 255},
        .meter_label = (Color){170, 220, 255, 255},
        .tick_counter = (Color){255, 255, 255, 255},
        .badge_text = (Color){140, 180, 220, 255},
        .axis_label = (Color){235, 235, 255, 255},
        .fault_pct_text = (Color){255, 255, 255, 255},
        .primary_text = (Color){220, 230, 255, 255},
        .knob_fill = (Color){255, 255, 255, 255},
        .mode_default = (Color){220, 230, 255, 255},
        .brak = (Color){90, 170, 220, 255},
        .ok = (Color){85, 255, 85, 255},
        .warning = (Color){255, 220, 90, 255},
        .shutdown = (Color){255, 96, 96, 255},
        .warning_fill = (Color){255, 220, 90, 200},
        .shutdown_fill = (Color){255, 96, 96, 210},
        .warning_dash = (Color){255, 220, 90, 170},
        .shutdown_dash = (Color){255, 96, 96, 170},
        .oil_shut_dash = (Color){255, 96, 96, 140},
        .warning_tint = (Color){255, 196, 64, 72},
        .shutdown_tint = (Color){255, 64, 64, 88},
        .speed_paused = (Color){255, 220, 90, 220},
        .end_notice = (Color){255, 240, 120, 255},
        .playhead = (Color){255, 255, 255, 220},
        .rpm = (Color){255, 170, 170, 255},
        .temp = (Color){255, 220, 90, 255},
        .oil = (Color){160, 200, 255, 255},
        .ctrl = (Color){85, 255, 255, 255},
        .slider_fill = (Color){85, 255, 255, 200},
        .slider_ring = (Color){170, 200, 240, 255},
        .replay_hint = (Color){170, 200, 240, 255},
    },
    [VISUALIZER_THEME_ONYX] = {
        .label = "ONYX",
        .window_bg = (Color){10, 12, 16, 255},
        .grad_top = (Color){18, 20, 26, 255},
        .grad_bot = (Color){6, 8, 12, 255},
        .panel_bg = (Color){16, 19, 25, 255},
        .nav_bg = (Color){13, 15, 21, 255},
        .timeline_bg = (Color){14, 17, 23, 255},
        .bar_bg = (Color){24, 28, 38, 255},
        .badge_bg = (Color){21, 24, 33, 255},
        .slider_track_bg = (Color){26, 30, 42, 255},
        .knob_shadow = (Color){0, 0, 0, 140},
        .hdr_border = (Color){58, 63, 78, 255},
        .panel_border = (Color){52, 58, 72, 255},
        .timeline_border = (Color){58, 64, 80, 255},
        .section_div = (Color){40, 46, 60, 255},
        .timeline_subdiv = (Color){44, 50, 66, 255},
        .fault_bar_bor = (Color){56, 61, 78, 255},
        .bar_border = (Color){60, 66, 84, 255},
        .slider_track_bor = (Color){72, 80, 100, 255},
        .badge_border = (Color){78, 88, 110, 255},
        .grid_line = (Color){64, 71, 88, 110},
        .grid_bright = (Color){140, 150, 172, 170},
        .grid_vert = (Color){54, 60, 78, 90},
        .caption = (Color){176, 156, 112, 255},
        .timeline_title = (Color){190, 198, 214, 255},
        .speed_running = (Color){90, 200, 188, 255},
        .scen_counter = (Color){142, 150, 168, 255},
        .sublabel = (Color){128, 136, 156, 255},
        .key = (Color){118, 214, 202, 255},
        .key_desc = (Color){124, 132, 150, 255},
        .meter_label = (Color){150, 158, 176, 255},
        .tick_counter = (Color){198, 206, 224, 255},
        .badge_text = (Color){214, 220, 236, 255},
        .axis_label = (Color){132, 140, 158, 255},
        .fault_pct_text = (Color){196, 204, 220, 255},
        .primary_text = (Color){245, 247, 250, 255},
        .knob_fill = (Color){238, 242, 248, 255},
        .mode_default = (Color){190, 198, 214, 255},
        .brak = (Color){98, 106, 126, 255},
        .ok = (Color){58, 199, 132, 255},
        .warning = (Color){246, 189, 96, 255},
        .shutdown = (Color){219, 78, 87, 255},
        .warning_fill = (Color){246, 189, 96, 190},
        .shutdown_fill = (Color){219, 78, 87, 200},
        .warning_dash = (Color){246, 189, 96, 135},
        .shutdown_dash = (Color){219, 78, 87, 145},
        .oil_shut_dash = (Color){219, 78, 87, 95},
        .warning_tint = (Color){246, 189, 96, 18},
        .shutdown_tint = (Color){219, 78, 87, 24},
        .speed_paused = (Color){246, 189, 96, 220},
        .end_notice = (Color){236, 220, 174, 255},
        .playhead = (Color){255, 255, 255, 220},
        .rpm = (Color){79, 179, 255, 255},
        .temp = (Color){255, 122, 122, 255},
        .oil = (Color){78, 204, 143, 255},
        .ctrl = (Color){94, 225, 205, 255},
        .slider_fill = (Color){118, 214, 202, 255},
        .slider_ring = (Color){118, 214, 202, 180},
        .replay_hint = (Color){154, 162, 182, 255},
    },
    [VISUALIZER_THEME_LIGHT] = {
        .label = "LIGHT",
        .window_bg = (Color){244, 247, 250, 255},
        .grad_top = (Color){252, 253, 255, 255},
        .grad_bot = (Color){232, 237, 243, 255},
        .panel_bg = (Color){250, 252, 255, 255},
        .nav_bg = (Color){240, 244, 249, 255},
        .timeline_bg = (Color){248, 250, 253, 255},
        .bar_bg = (Color){226, 232, 240, 255},
        .badge_bg = (Color){236, 241, 247, 255},
        .slider_track_bg = (Color){216, 224, 234, 255},
        .knob_shadow = (Color){80, 98, 120, 50},
        .hdr_border = (Color){173, 183, 198, 255},
        .panel_border = (Color){180, 190, 204, 255},
        .timeline_border = (Color){173, 184, 198, 255},
        .section_div = (Color){194, 202, 214, 255},
        .timeline_subdiv = (Color){198, 206, 218, 255},
        .fault_bar_bor = (Color){168, 179, 193, 255},
        .bar_border = (Color){160, 171, 186, 255},
        .slider_track_bor = (Color){150, 163, 180, 255},
        .badge_border = (Color){145, 158, 176, 255},
        .grid_line = (Color){182, 191, 204, 130},
        .grid_bright = (Color){108, 121, 141, 200},
        .grid_vert = (Color){190, 198, 210, 120},
        .caption = (Color){86, 97, 114, 255},
        .timeline_title = (Color){70, 80, 98, 255},
        .speed_running = (Color){39, 121, 118, 255},
        .scen_counter = (Color){92, 102, 120, 255},
        .sublabel = (Color){95, 106, 124, 255},
        .key = (Color){36, 118, 180, 255},
        .key_desc = (Color){90, 101, 120, 255},
        .meter_label = (Color){66, 77, 94, 255},
        .tick_counter = (Color){62, 73, 91, 255},
        .badge_text = (Color){66, 77, 94, 255},
        .axis_label = (Color){84, 95, 114, 255},
        .fault_pct_text = (Color){70, 81, 96, 255},
        .primary_text = (Color){32, 43, 56, 255},
        .knob_fill = (Color){252, 253, 255, 255},
        .mode_default = (Color){85, 97, 114, 255},
        .brak = (Color){118, 129, 146, 255},
        .ok = (Color){34, 146, 88, 255},
        .warning = (Color){190, 132, 20, 255},
        .shutdown = (Color){190, 60, 76, 255},
        .warning_fill = (Color){190, 132, 20, 180},
        .shutdown_fill = (Color){190, 60, 76, 190},
        .warning_dash = (Color){190, 132, 20, 140},
        .shutdown_dash = (Color){190, 60, 76, 150},
        .oil_shut_dash = (Color){190, 60, 76, 110},
        .warning_tint = (Color){190, 132, 20, 18},
        .shutdown_tint = (Color){190, 60, 76, 22},
        .speed_paused = (Color){176, 120, 24, 255},
        .end_notice = (Color){129, 99, 23, 255},
        .playhead = (Color){46, 59, 79, 220},
        .rpm = (Color){38, 132, 214, 255},
        .temp = (Color){230, 96, 114, 255},
        .oil = (Color){42, 162, 92, 255},
        .ctrl = (Color){89, 106, 214, 255},
        .slider_fill = (Color){49, 124, 199, 255},
        .slider_ring = (Color){49, 124, 199, 170},
        .replay_hint = (Color){92, 103, 121, 255},
    },
};

static VisualizerThemeId g_visualizer_theme = VISUALIZER_THEME_MIDNIGHT;

static const VisualizerTheme *visualizer_active_theme(void)
{
    return &k_visualizer_themes[g_visualizer_theme];
}

/* Background layers */
#define COL_WINDOW_BG (visualizer_active_theme()->window_bg)
#define COL_GRAD_TOP (visualizer_active_theme()->grad_top)
#define COL_GRAD_BOT (visualizer_active_theme()->grad_bot)
#define COL_PANEL_BG (visualizer_active_theme()->panel_bg)
#define COL_NAV_BG (visualizer_active_theme()->nav_bg)
#define COL_TIMELINE_BG (visualizer_active_theme()->timeline_bg)
#define COL_BAR_BG (visualizer_active_theme()->bar_bg)
#define COL_BADGE_BG (visualizer_active_theme()->badge_bg)
#define COL_SLIDER_TRACK_BG (visualizer_active_theme()->slider_track_bg)
#define COL_KNOB_SHADOW (visualizer_active_theme()->knob_shadow)
/* Borders / dividers */
#define COL_HDR_BORDER (visualizer_active_theme()->hdr_border)
#define COL_PANEL_BORDER (visualizer_active_theme()->panel_border)
#define COL_TIMELINE_BORDER (visualizer_active_theme()->timeline_border)
#define COL_SECTION_DIV (visualizer_active_theme()->section_div)
#define COL_TIMELINE_SUBDIV (visualizer_active_theme()->timeline_subdiv)
#define COL_FAULT_BAR_BOR (visualizer_active_theme()->fault_bar_bor)
#define COL_BAR_BORDER (visualizer_active_theme()->bar_border)
#define COL_SLIDER_TRACK_BOR (visualizer_active_theme()->slider_track_bor)
#define COL_BADGE_BORDER (visualizer_active_theme()->badge_border)
/* Grid */
#define COL_GRID_LINE (visualizer_active_theme()->grid_line)
#define COL_GRID_BRIGHT (visualizer_active_theme()->grid_bright)
#define COL_GRID_VERT (visualizer_active_theme()->grid_vert)
/* Text - dark to light */
#define COL_CAPTION (visualizer_active_theme()->caption)
#define COL_TIMELINE_TITLE (visualizer_active_theme()->timeline_title)
#define COL_SPEED_RUNNING (visualizer_active_theme()->speed_running)
#define COL_SCEN_COUNTER (visualizer_active_theme()->scen_counter)
#define COL_SUBLABEL (visualizer_active_theme()->sublabel)
#define COL_KEY (visualizer_active_theme()->key)
#define COL_KEY_DESC (visualizer_active_theme()->key_desc)
#define COL_METER_LABEL (visualizer_active_theme()->meter_label)
#define COL_TICK_COUNTER (visualizer_active_theme()->tick_counter)
#define COL_BADGE_TEXT (visualizer_active_theme()->badge_text)
#define COL_AXIS_LABEL (visualizer_active_theme()->axis_label)
#define COL_FAULT_PCT_TEXT (visualizer_active_theme()->fault_pct_text)
#define COL_PRIMARY_TEXT (visualizer_active_theme()->primary_text)
#define COL_KNOB_FILL (visualizer_active_theme()->knob_fill)
#define COL_MODE_DEFAULT (visualizer_active_theme()->mode_default)
#define COL_BRAK (visualizer_active_theme()->brak)
/* Semantic status */
#define COL_OK (visualizer_active_theme()->ok)
#define COL_WARNING (visualizer_active_theme()->warning)
#define COL_SHUTDOWN (visualizer_active_theme()->shutdown)
#define COL_WARNING_FILL (visualizer_active_theme()->warning_fill)
#define COL_SHUTDOWN_FILL (visualizer_active_theme()->shutdown_fill)
#define COL_WARNING_DASH (visualizer_active_theme()->warning_dash)
#define COL_SHUTDOWN_DASH (visualizer_active_theme()->shutdown_dash)
#define COL_OIL_SHUT_DASH (visualizer_active_theme()->oil_shut_dash)
#define COL_WARNING_TINT (visualizer_active_theme()->warning_tint)
#define COL_SHUTDOWN_TINT (visualizer_active_theme()->shutdown_tint)
#define COL_SPEED_PAUSED (visualizer_active_theme()->speed_paused)
#define COL_END_NOTICE (visualizer_active_theme()->end_notice)
#define COL_PLAYHEAD (visualizer_active_theme()->playhead)
/* Signal / sensor lines */
#define COL_RPM (visualizer_active_theme()->rpm)
#define COL_TEMP (visualizer_active_theme()->temp)
#define COL_OIL (visualizer_active_theme()->oil)
#define COL_CTRL (visualizer_active_theme()->ctrl)
/* Slider accent */
#define COL_SLIDER_FILL (visualizer_active_theme()->slider_fill)
#define COL_SLIDER_RING (visualizer_active_theme()->slider_ring)

#define FS_TINY 14.0f
#define FS_SMALL 14.0f
#define FS_KEY_HINT 14.0f
#define FS_VALUE 14.0f
#define FS_BADGE 14.0f
#define FS_MODE 28.0f
#define FS_SCEN_NAME 28.0f
#define FS_TL_TITLE 14.0f
#define FS_TL_LEGEND 14.0f
#define FS_TL_AXIS 14.0f

#define PIXEL_FONT_HEIGHT 14.0f
#define PIXEL_FONT_SPACING 0.0f

#define LAYOUT_PAD 16.0f
#define LAYOUT_HDR_H 64.0f
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
#define LAYOUT_SLIDER_INSET 40.0f
#define LAYOUT_SLIDER_H 24.0f
#define LAYOUT_TIMELINE_HEADER_H 28.0f
#define LAYOUT_TIMELINE_TITLE_X 16.0f
#define LAYOUT_TIMELINE_TITLE_Y 6.0f
#define LAYOUT_TIMELINE_PLOT_LEFT 52.0f
#define LAYOUT_TIMELINE_PLOT_RIGHT 18.0f
#define LAYOUT_TIMELINE_PLOT_TOP 40.0f
#define LAYOUT_TIMELINE_PLOT_BOTTOM 54.0f
#define LAYOUT_TIMELINE_AXIS_X 10.0f
#define LAYOUT_TIMELINE_AXIS_Y_NUDGE 7.0f
#define LAYOUT_TIMELINE_XLABEL_Y 10.0f
#define LAYOUT_TIMELINE_LEGEND_RIGHT 16.0f
#define LAYOUT_TIMELINE_LEGEND_Y 6.0f
#define LAYOUT_HEADER_TOP_Y 16.0f
#define LAYOUT_HEADER_MAIN_Y 32.0f
#define LAYOUT_HEADER_SEGMENT_GAP 24.0f
#define LAYOUT_HEADER_META_GAP 18.0f
#define LAYOUT_STATUS_MODE_X 14.0f
#define LAYOUT_STATUS_RESULT_X_PCT 0.33f
#define LAYOUT_STATUS_RUN_RIGHT 120.0f
#define LAYOUT_STATUS_CAP_Y 40.0f
#define LAYOUT_STATUS_VALUE_GAP 18.0f
#define LAYOUT_STATUS_FAULT_DIV_Y 126.0f
#define LAYOUT_STATUS_FAULT_TITLE_Y 136.0f
#define LAYOUT_STATUS_FAULT_ROW0_Y 154.0f
#define LAYOUT_STATUS_FAULT_ROW1_Y 170.0f
#define LAYOUT_STATUS_END_DIV_Y 196.0f
#define LAYOUT_STATUS_END_TITLE_Y 206.0f
#define LAYOUT_STATUS_END_HINT_Y 222.0f

#define COL_REPLAY_HINT (visualizer_active_theme()->replay_hint)

int visualizer_parse_theme_id(const char *name, VisualizerThemeId *theme_id)
{
    if ((name == NULL) || (theme_id == NULL))
    {
        return 0;
    }

    if ((strcmp(name, "default") == 0) || (strcmp(name, "midnight") == 0))
    {
        *theme_id = VISUALIZER_THEME_MIDNIGHT;
        return 1;
    }

    if ((strcmp(name, "dos") == 0) || (strcmp(name, "dos-blue") == 0) || (strcmp(name, "dos_blue") == 0))
    {
        *theme_id = VISUALIZER_THEME_DOS_BLUE;
        return 1;
    }

    if ((strcmp(name, "onyx") == 0) || (strcmp(name, "modern") == 0) || (strcmp(name, "premium") == 0))
    {
        *theme_id = VISUALIZER_THEME_ONYX;
        return 1;
    }

    if ((strcmp(name, "light") == 0) || (strcmp(name, "light-mode") == 0) || (strcmp(name, "day") == 0))
    {
        *theme_id = VISUALIZER_THEME_LIGHT;
        return 1;
    }

    return 0;
}

const char *visualizer_theme_label(VisualizerThemeId theme_id)
{
    if ((theme_id < 0) || (theme_id >= VISUALIZER_THEME_COUNT))
    {
        return k_visualizer_themes[VISUALIZER_THEME_MIDNIGHT].label;
    }

    return k_visualizer_themes[theme_id].label;
}

VisualizerThemeId visualizer_theme_get(void)
{
    return g_visualizer_theme;
}

VisualizerThemeId visualizer_theme_cycle(void)
{
    g_visualizer_theme = (VisualizerThemeId)((g_visualizer_theme + 1) % VISUALIZER_THEME_COUNT);
    return g_visualizer_theme;
}

void visualizer_theme_set(VisualizerThemeId theme_id)
{
    if ((theme_id < 0) || (theme_id >= VISUALIZER_THEME_COUNT))
    {
        g_visualizer_theme = VISUALIZER_THEME_MIDNIGHT;
        return;
    }

    g_visualizer_theme = theme_id;
}

static void draw_text_font(const Font *font, const char *text, float x, float y, float size, Color color)
{
    float snapped_size;

    if ((font == NULL) || (text == NULL))
    {
        return;
    }

    snapped_size = PIXEL_FONT_HEIGHT * roundf(size / PIXEL_FONT_HEIGHT);
    if (snapped_size < PIXEL_FONT_HEIGHT)
    {
        snapped_size = PIXEL_FONT_HEIGHT;
    }

    DrawTextEx(*font, text, (Vector2){roundf(x), roundf(y)}, snapped_size, PIXEL_FONT_SPACING, color);
}

static Vector2 measure_text_font(const Font *font, const char *text, float size)
{
    float snapped_size;

    if ((font == NULL) || (text == NULL))
    {
        return (Vector2){0.0f, 0.0f};
    }

    snapped_size = PIXEL_FONT_HEIGHT * roundf(size / PIXEL_FONT_HEIGHT);
    if (snapped_size < PIXEL_FONT_HEIGHT)
    {
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
    char expected_text[64];
    Vector2 name_sz;
    Vector2 req_sz;
    Vector2 exp_sz;
    float preferred_fs = FS_SCEN_NAME * scale;
    float compact_fs = FS_TINY * scale;

    if ((font == NULL) || (scenario_name == NULL) || (requirement_id == NULL) || (expected == NULL))
    {
        return compact_fs;
    }

    (void)snprintf(expected_text, sizeof(expected_text), "EXPECTED: %s", expected);
    name_sz = measure_text_font(font, scenario_name, preferred_fs);
    req_sz = measure_text_font(font, requirement_id, compact_fs);
    exp_sz = measure_text_font(font, expected_text, compact_fs);

    if ((name_sz.x + LAYOUT_HEADER_SEGMENT_GAP * scale + req_sz.x +
         LAYOUT_HEADER_META_GAP * scale + exp_sz.x) <= available_width)
    {
        return preferred_fs;
    }

    return compact_fs;
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

    sz = measure_text_font(font, title, title_fs);
    draw_text_font(font, title,
                   box_x + (box_w - sz.x) * 0.5f,
                   box_y + 14.0f * scale,
                   title_fs, COL_PRIMARY_TEXT);

    DrawLine((int)(box_x + 16.0f * scale), (int)(box_y + 54.0f * scale),
             (int)(box_x + box_w - 16.0f * scale), (int)(box_y + 54.0f * scale),
             COL_SECTION_DIV);

    yes_bg = COL_BADGE_BG;
    yes_bor = (selection == 1) ? COL_PRIMARY_TEXT : COL_BADGE_BORDER;
    yes_txt = COL_PRIMARY_TEXT;
    DrawRectangle((int)yes_x, (int)btn_y, (int)btn_w, (int)btn_h, yes_bg);
    DrawRectangleLines((int)yes_x, (int)btn_y, (int)btn_w, (int)btn_h, yes_bor);
    sz = measure_text_font(font, "YES", opt_fs);
    draw_text_font(font, "YES",
                   yes_x + (btn_w - sz.x) * 0.5f,
                   btn_y + (btn_h - sz.y) * 0.5f,
                   opt_fs, yes_txt);

    no_bg = COL_BADGE_BG;
    no_bor = (selection == 0) ? COL_PRIMARY_TEXT : COL_BADGE_BORDER;
    no_txt = COL_PRIMARY_TEXT;
    DrawRectangle((int)no_x, (int)btn_y, (int)btn_w, (int)btn_h, no_bg);
    DrawRectangleLines((int)no_x, (int)btn_y, (int)btn_w, (int)btn_h, no_bor);
    sz = measure_text_font(font, "NO", opt_fs);
    draw_text_font(font, "NO",
                   no_x + (btn_w - sz.x) * 0.5f,
                   btn_y + (btn_h - sz.y) * 0.5f,
                   opt_fs, no_txt);

    sz = measure_text_font(font, hint, hint_fs);
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
    value_size = measure_text_font(font, value_text, val_fs);
    value_x = value_area.x + value_area.width - value_size.x;
    if (value_x < value_area.x)
    {
        value_x = value_area.x;
    }
    value_y = bar_area.y + (bar_area.height - val_fs) * 0.5f;
    draw_text_font(font, value_text, value_x, value_y, val_fs, COL_PRIMARY_TEXT);
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

    {
        float title_fs = FS_TL_TITLE * scale;
        draw_text_font(font, "TIMELINE",
                       area.x + LAYOUT_TIMELINE_TITLE_X * scale,
                       area.y + LAYOUT_TIMELINE_TITLE_Y * scale,
                       title_fs, COL_TIMELINE_TITLE);
    }
    DrawLine((int)(area.x + 1), (int)(area.y + LAYOUT_TIMELINE_HEADER_H * scale),
             (int)(area.x + area.width - 1), (int)(area.y + LAYOUT_TIMELINE_HEADER_H * scale),
             COL_TIMELINE_SUBDIV);

    {
        unsigned int active_tick = scenario->ticks[(unsigned int)(playhead + 0.5f)].tick;
        char tick_label[48];
        Vector2 tl_sz;
        float tl_x;
        float title_fs = FS_TL_TITLE * scale;
        (void)snprintf(tick_label, sizeof(tick_label), "Tick %u / %u", active_tick,
                       scenario->ticks[scenario->tick_count - 1U].tick);
        tl_sz = measure_text_font(font, tick_label, title_fs);
        tl_x = area.x + area.width * 0.5f - tl_sz.x * 0.5f;
        draw_text_font(font, tick_label,
                       tl_x,
                       area.y + LAYOUT_TIMELINE_TITLE_Y * scale,
                       title_fs, COL_TICK_COUNTER);
    }

    plot_left = (int)(area.x + LAYOUT_TIMELINE_PLOT_LEFT * scale);
    plot_right = (int)(area.x + area.width - LAYOUT_TIMELINE_PLOT_RIGHT * scale);
    plot_top = (int)(area.y + LAYOUT_TIMELINE_PLOT_TOP * scale);
    plot_bottom = (int)(area.y + area.height - LAYOUT_TIMELINE_PLOT_BOTTOM * scale);
    plot_w = plot_right - plot_left;
    plot_h = plot_bottom - plot_top;

    for (i = 0; i <= 8; ++i)
    {
        int gy = plot_top + (i * plot_h) / 8;
        float axis_fs = FS_TL_AXIS * scale;
        Color grid_color = (i == 8) ? COL_GRID_BRIGHT : COL_GRID_LINE;
        DrawLine(plot_left, gy, plot_right, gy, grid_color);
        if ((i % 2) == 0)
        {
            char y_label[16];
            (void)snprintf(y_label, sizeof(y_label), "%d%%", 100 - ((i * 100) / 8));
            draw_text_font(font, y_label,
                           area.x + LAYOUT_TIMELINE_AXIS_X * scale,
                           (float)gy - LAYOUT_TIMELINE_AXIS_Y_NUDGE * scale,
                           axis_fs, COL_AXIS_LABEL);
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
            float axis_fs = FS_TL_AXIS * scale;
            DrawLine(gx, plot_top, gx, plot_bottom, COL_GRID_VERT);
            (void)snprintf(x_label, sizeof(x_label), "%u", scenario->ticks[tick_idx].tick);
            draw_text_font(font, x_label,
                           (float)gx - 8.0f * scale,
                           (float)plot_bottom + LAYOUT_TIMELINE_XLABEL_Y * scale,
                           axis_fs, COL_AXIS_LABEL);
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
        float ley = area.y + LAYOUT_TIMELINE_LEGEND_Y * scale;
        float lfs2 = FS_TL_LEGEND * scale;
        float dot_r = LAYOUT_DOT_R * scale;
        float lx = area.x + area.width - LAYOUT_TIMELINE_LEGEND_RIGHT * scale;
        Vector2 lsz;
        static const char *k_legend_names[] = {
            "CTRL",
            "OIL",
            "TEMP",
            "RPM",
        };
        int li;

        for (li = 0; li < 4; ++li)
        {
            Color legend_color = COL_CTRL;

            if (li == 1)
            {
                legend_color = COL_OIL;
            }
            else if (li == 2)
            {
                legend_color = COL_TEMP;
            }
            else if (li == 3)
            {
                legend_color = COL_RPM;
            }

            lsz = measure_text_font(font, k_legend_names[li], lfs2);
            lx -= lsz.x;
            draw_text_font(font, k_legend_names[li], lx, ley, lfs2, legend_color);
            lx -= dot_r * 2.0f + LAYOUT_LEGEND_DOT_GAP * scale;
            DrawCircle((int)(lx + dot_r), (int)(ley + lfs2 * 0.5f), dot_r, legend_color);
            lx -= LAYOUT_LEGEND_COL_GAP * scale;
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

    timeline_y = content_y + main_h + 14.0f * layout->scale;
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
        char expected_text[64];
        char tick_str[48];
        char theme_str[48];
        Vector2 req_sz;
        Vector2 exp_sz;
        Vector2 tick_sz;
        Vector2 scen_sz;
        Vector2 theme_sz;
        float header_text_right;
        float scen_fs;
        float meta_fs;
        float req_x;
        float exp_x;
        float main_row_y;
        float meta_row_y;
        float meta_x;
        float badge_w;
        float badge_x;
        float badge_y;
        float theme_badge_w;
        float theme_badge_x;
        float badge_h = LAYOUT_BADGE_H * layout->scale;

        (void)snprintf(scen_label, sizeof(scen_label),
                       "SCENARIO  %u / %u",
                       scenario_set->active_index + 1U, scenario_set->count);
        draw_text_font(font, scen_label,
                       layout->pad,
                       LAYOUT_HEADER_TOP_Y * layout->scale,
                       FS_TINY * layout->scale,
                       COL_SCEN_COUNTER);
        (void)snprintf(tick_str, sizeof(tick_str), "Tick  %u / %u",
                       tick->tick, scenario->tick_count);
        tick_sz = measure_text_font(font, tick_str, FS_BADGE * layout->scale);
        badge_w = tick_sz.x + 20.0f * layout->scale;
        badge_x = (float)screen_w - badge_w - layout->pad;
        badge_y = 8.0f * layout->scale;

        (void)snprintf(theme_str, sizeof(theme_str), "THEME  %s", visualizer_theme_label(visualizer_theme_get()));
        theme_sz = measure_text_font(font, theme_str, FS_BADGE * layout->scale);
        theme_badge_w = theme_sz.x + 20.0f * layout->scale;
        theme_badge_x = badge_x - theme_badge_w - 10.0f * layout->scale;

        header_text_right = theme_badge_x - 20.0f * layout->scale;
        meta_fs = FS_TINY * layout->scale;
        scen_fs = pick_header_name_font_size(font,
                                             scenario->scenario,
                                             scenario->requirement_id,
                                             scenario->expected,
                                             header_text_right - layout->pad,
                                             layout->scale);
        main_row_y = LAYOUT_HEADER_MAIN_Y * layout->scale;
        meta_row_y = main_row_y + (scen_fs - meta_fs);

        draw_text_font(font, scenario->scenario,
                       layout->pad,
                       main_row_y,
                       scen_fs,
                       COL_PRIMARY_TEXT);
        scen_sz = measure_text_font(font, scenario->scenario, scen_fs);
        req_x = layout->pad + scen_sz.x + LAYOUT_HEADER_SEGMENT_GAP * layout->scale;
        exp_x = req_x;

        req_sz = measure_text_font(font, scenario->requirement_id, meta_fs);
        if ((req_x + req_sz.x) < header_text_right)
        {
            draw_text_font(font, scenario->requirement_id,
                           req_x,
                           meta_row_y,
                           meta_fs,
                           COL_SCEN_COUNTER);
            exp_x = req_x + req_sz.x + LAYOUT_HEADER_META_GAP * layout->scale;
        }

        (void)snprintf(expected_text, sizeof(expected_text), "EXPECTED: %s", scenario->expected);
        exp_sz = measure_text_font(font, expected_text, meta_fs);
        meta_x = exp_x;
        if ((meta_x + exp_sz.x) < header_text_right)
        {
            draw_text_font(font, expected_text,
                           meta_x,
                           meta_row_y,
                           meta_fs,
                           COL_SCEN_COUNTER);
        }

        DrawRectangle((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h, COL_BADGE_BG);
        DrawRectangleLines((int)badge_x, (int)badge_y, (int)badge_w, (int)badge_h, COL_BADGE_BORDER);
        draw_text_font(font, tick_str,
                       badge_x + 10.0f * layout->scale,
                       badge_y + (badge_h - FS_BADGE * layout->scale) * 0.5f,
                       FS_BADGE * layout->scale, COL_BADGE_TEXT);

        if (theme_badge_x > (meta_x + 24.0f * layout->scale))
        {
            DrawRectangle((int)theme_badge_x, (int)badge_y, (int)theme_badge_w, (int)badge_h, COL_BADGE_BG);
            DrawRectangleLines((int)theme_badge_x, (int)badge_y, (int)theme_badge_w, (int)badge_h, COL_BADGE_BORDER);
            draw_text_font(font, theme_str,
                           theme_badge_x + 10.0f * layout->scale,
                           badge_y + (badge_h - FS_BADGE * layout->scale) * 0.5f,
                           FS_BADGE * layout->scale, COL_BADGE_TEXT);
        }
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
        kx += draw_key_hint(font, "T", "Theme", kx, ky, kfs) + gap2;
        kx += draw_key_hint(font, "F11", "Fullscreen", kx, ky, kfs) + gap2;
        kx += draw_key_hint(font, "ESC", "Quit", kx, ky, kfs);
        (void)kx;

        (void)snprintf(speed_str, sizeof(speed_str), "%.0f tk/s%s",
                       ticks_per_second, (paused != 0) ? "  PAUSED" : "");
        sp_sz = measure_text_font(font, speed_str, kfs);
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
        float lx = layout->metrics_area.x + LAYOUT_STATUS_MODE_X * layout->scale;
        float rx = layout->metrics_area.x + layout->metrics_area.width * LAYOUT_STATUS_RESULT_X_PCT;
        float run_col_x = layout->metrics_area.x + layout->metrics_area.width - LAYOUT_STATUS_RUN_RIGHT * layout->scale;
        float cap_y = layout->metrics_area.y + LAYOUT_STATUS_CAP_Y * layout->scale;
        float cap_fs = FS_TINY * layout->scale;
        float mode_fs = FS_VALUE * layout->scale;
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
        result_y = cap_y + LAYOUT_STATUS_VALUE_GAP * layout->scale;
        draw_text_font(font, tick->result, rx, result_y, mode_fs, result_c);

        (void)snprintf(run_text, sizeof(run_text), "%d", tick->run);
        draw_text_font(font, run_text, run_col_x, result_y, mode_fs, result_c);
    }

    DrawLine((int)(layout->metrics_area.x + 14.0f * layout->scale), (int)(layout->metrics_area.y + LAYOUT_STATUS_FAULT_DIV_Y * layout->scale),
             (int)(layout->metrics_area.x + layout->metrics_area.width - 14.0f * layout->scale),
             (int)(layout->metrics_area.y + LAYOUT_STATUS_FAULT_DIV_Y * layout->scale),
             COL_SECTION_DIV);

    draw_text_font(font, "SESSION FAULT RATE",
                   layout->metrics_area.x + 14.0f * layout->scale, layout->metrics_area.y + LAYOUT_STATUS_FAULT_TITLE_Y * layout->scale,
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
                            layout->metrics_area.y + LAYOUT_STATUS_FAULT_ROW0_Y * layout->scale,
                            COL_WARNING_FILL, layout->scale);
        draw_fault_rate_row(font, "SHUTDOWN", shutdown_pct,
                            bx2, tr_x, tr_w, bh2,
                            layout->metrics_area.y + LAYOUT_STATUS_FAULT_ROW1_Y * layout->scale,
                            COL_SHUTDOWN_FILL, layout->scale);
    }

    if (playhead >= (float)(scenario->tick_count - 1U))
    {
        DrawLine((int)(layout->metrics_area.x + 14.0f * layout->scale), (int)(layout->metrics_area.y + LAYOUT_STATUS_END_DIV_Y * layout->scale),
                 (int)(layout->metrics_area.x + layout->metrics_area.width - 14.0f * layout->scale),
                 (int)(layout->metrics_area.y + LAYOUT_STATUS_END_DIV_Y * layout->scale),
                 COL_SECTION_DIV);
        draw_text_font(font, "End of scenario",
                       layout->metrics_area.x + 14.0f * layout->scale, layout->metrics_area.y + LAYOUT_STATUS_END_TITLE_Y * layout->scale,
                       FS_SMALL * layout->scale, COL_END_NOTICE);
        draw_text_font(font, "Press  R  to replay",
                       layout->metrics_area.x + 14.0f * layout->scale, layout->metrics_area.y + LAYOUT_STATUS_END_HINT_Y * layout->scale,
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