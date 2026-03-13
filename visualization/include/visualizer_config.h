#ifndef VISUALIZER_CONFIG_H
#define VISUALIZER_CONFIG_H

#include "raylib.h"

typedef enum {
	VISUALIZER_THEME_DEFAULT = 0,
	VISUALIZER_THEME_DOS_BLUE,
	VISUALIZER_THEME_ONYX,
	VISUALIZER_THEME_GRUVBOX_DARK,
	VISUALIZER_THEME_LIGHT,
	VISUALIZER_THEME_COUNT
} VisualizerThemeId;

typedef struct {
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

extern const float FS_TINY;
extern const float FS_SMALL;
extern const float FS_KEY_HINT;
extern const float FS_VALUE;
extern const float FS_BADGE;
extern const float FS_MODE;
extern const float FS_SCEN_NAME;
extern const float FS_TL_TITLE;
extern const float FS_TL_LEGEND;
extern const float FS_TL_AXIS;

extern const float LAYOUT_PAD;
extern const float LAYOUT_HDR_H;
extern const float LAYOUT_NAV_H;
extern const float LAYOUT_MAIN_H;
extern const float LAYOUT_STATUS_W;
extern const float LAYOUT_COL_GAP;
extern const float LAYOUT_BAR_H;
extern const float LAYOUT_ROW_STEP;
extern const float LAYOUT_BADGE_H;
extern const float LAYOUT_KNOB_R;
extern const float LAYOUT_KNOB_SHADOW;
extern const float LAYOUT_DOT_R;
extern const float LAYOUT_LEGEND_DOT_GAP;
extern const float LAYOUT_LEGEND_COL_GAP;
extern const float LAYOUT_SLIDER_INSET;
extern const float LAYOUT_SLIDER_H;
extern const float LAYOUT_HEADER_TOP_Y;
extern const float LAYOUT_HEADER_MAIN_Y;
extern const float LAYOUT_HEADER_SEGMENT_GAP;
extern const float LAYOUT_HEADER_META_GAP;

extern const float TL_HEADER_H;
extern const float TL_TITLE_X;
extern const float TL_TITLE_Y;
extern const float LAYOUT_TIMELINE_PLOT_LEFT;
extern const float LAYOUT_TIMELINE_PLOT_RIGHT;
extern const float LAYOUT_TIMELINE_PLOT_TOP;
extern const float LAYOUT_TIMELINE_AXIS_X;
extern const float LAYOUT_TIMELINE_AXIS_Y_NUDGE;
extern const float LAYOUT_TIMELINE_XLABEL_Y;
extern const float LAYOUT_TIMELINE_XLABEL_BAND;
extern const float LAYOUT_TIMELINE_LEGEND_RIGHT;
extern const float LAYOUT_TIMELINE_LEGEND_Y;
extern const float LAYOUT_TIMELINE_SLIDER_BOTTOM_PAD;

extern const float LAYOUT_STATUS_MODE_X;
extern const float LAYOUT_STATUS_RESULT_X_PCT;
extern const float LAYOUT_STATUS_RUN_RIGHT;
extern const float LAYOUT_STATUS_CAP_Y;
extern const float LAYOUT_STATUS_VALUE_GAP;
extern const float LAYOUT_STATUS_FAULT_DIV_Y;
extern const float LAYOUT_STATUS_FAULT_TITLE_Y;
extern const float LAYOUT_STATUS_FAULT_ROW0_Y;
extern const float LAYOUT_STATUS_FAULT_ROW1_Y;
extern const float LAYOUT_STATUS_FAULT_LABEL_GAP;
extern const float LAYOUT_STATUS_END_DIV_Y;
extern const float LAYOUT_STATUS_END_TITLE_Y;
extern const float LAYOUT_STATUS_END_HINT_Y;

const VisualizerTheme *visualizer_active_theme(void);
int visualizer_parse_theme_id(const char *name, VisualizerThemeId *theme_id);
const char *visualizer_theme_label(VisualizerThemeId theme_id);
VisualizerThemeId visualizer_theme_get(void);
VisualizerThemeId visualizer_theme_cycle(void);
void visualizer_theme_set(VisualizerThemeId theme_id);

#endif
