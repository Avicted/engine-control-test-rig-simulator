#include <string.h>

#include "visualizer_config.h"

static const VisualizerTheme k_visualizer_themes[VISUALIZER_THEME_COUNT] = {
	[VISUALIZER_THEME_DEFAULT] = {
		.label            = "DEFAULT",
		.window_bg        = (Color){9, 11, 18, 255},
		.grad_top         = (Color){11, 14, 22, 255},
		.grad_bot         = (Color){7, 9, 16, 255},
		.panel_bg         = (Color){13, 16, 26, 255},
		.nav_bg           = (Color){10, 12, 20, 255},
		.timeline_bg      = (Color){12, 15, 24, 255},
		.bar_bg           = (Color){18, 22, 34, 255},
		.badge_bg         = (Color){20, 25, 40, 255},
		.slider_track_bg  = (Color){22, 27, 44, 255},
		.knob_shadow      = (Color){8, 10, 18, 180},
		.hdr_border       = (Color){30, 36, 58, 255},
		.panel_border     = (Color){30, 36, 56, 255},
		.timeline_border  = (Color){32, 38, 58, 255},
		.section_div      = (Color){24, 29, 48, 255},
		.timeline_subdiv  = (Color){26, 31, 50, 255},
		.fault_bar_bor    = (Color){38, 44, 64, 255},
		.bar_border       = (Color){40, 46, 66, 255},
		.slider_track_bor = (Color){44, 52, 76, 255},
		.badge_border     = (Color){44, 54, 84, 255},
		.grid_line        = (Color){44, 49, 63, 180},
		.grid_bright      = (Color){96, 104, 122, 220},
		.grid_vert        = (Color){38, 43, 58, 170},
		.caption          = (Color){68, 82, 118, 255},
		.timeline_title   = (Color){72, 85, 120, 255},
		.speed_running    = (Color){72, 86, 120, 255},
		.scen_counter     = (Color){78, 94, 136, 255},
		.sublabel         = (Color){90, 106, 148, 255},
		.key              = (Color){90, 170, 255, 255},
		.key_desc         = (Color){110, 124, 155, 255},
		.meter_label      = (Color){120, 134, 165, 255},
		.tick_counter     = (Color){140, 155, 186, 255},
		.badge_text       = (Color){148, 165, 205, 255},
		.axis_label       = (Color){160, 168, 184, 255},
		.fault_pct_text   = (Color){170, 182, 212, 255},
		.primary_text     = (Color){245, 248, 255, 255},
		.knob_fill        = (Color){220, 228, 245, 255},
		.mode_default     = (Color){180, 186, 200, 255},
		.brak             = (Color){50, 62, 94, 255},
		.ok               = (Color){40, 167, 69, 255},
		.warning          = (Color){255, 193, 7, 255},
		.shutdown         = (Color){220, 53, 69, 255},
		.warning_fill     = (Color){255, 193, 7, 200},
		.shutdown_fill    = (Color){220, 53, 69, 210},
		.warning_dash     = (Color){255, 193, 7, 160},
		.shutdown_dash    = (Color){220, 53, 69, 170},
		.oil_shut_dash    = (Color){220, 53, 69, 120},
		.warning_tint     = (Color){255, 193, 7, 26},
		.shutdown_tint    = (Color){220, 53, 69, 34},
		.speed_paused     = (Color){255, 193, 7, 220},
		.end_notice       = (Color){255, 220, 100, 255},
		.playhead         = (Color){255, 255, 255, 210},
		.rpm              = (Color){52, 152, 219, 255},
		.temp             = (Color){255, 99, 132, 255},
		.oil              = (Color){46, 204, 113, 255},
		.ctrl             = (Color){176, 132, 255, 255},
		.slider_fill      = (Color){48, 128, 220, 255},
		.slider_ring      = (Color){48, 128, 220, 200},
		.replay_hint      = (Color){160, 174, 210, 255},
	},
	[VISUALIZER_THEME_DOS_BLUE] = {
		.label            = "DOS BLUE",
		.window_bg        = (Color){0, 0, 170, 255},
		.grad_top         = (Color){0, 0, 184, 255},
		.grad_bot         = (Color){0, 0, 136, 255},
		.panel_bg         = (Color){0, 0, 164, 255},
		.nav_bg           = (Color){0, 0, 144, 255},
		.timeline_bg      = (Color){0, 0, 152, 255},
		.bar_bg           = (Color){0, 0, 112, 255},
		.badge_bg         = (Color){0, 0, 136, 255},
		.slider_track_bg  = (Color){0, 0, 104, 255},
		.knob_shadow      = (Color){0, 0, 0, 120},
		.hdr_border       = (Color){60, 120, 220, 255},
		.panel_border     = (Color){60, 120, 220, 255},
		.timeline_border  = (Color){60, 120, 220, 255},
		.section_div      = (Color){50, 100, 200, 255},
		.timeline_subdiv  = (Color){70, 140, 220, 255},
		.fault_bar_bor    = (Color){60, 100, 180, 255},
		.bar_border       = (Color){60, 100, 180, 255},
		.slider_track_bor = (Color){90, 170, 220, 255},
		.badge_border     = (Color){90, 170, 220, 255},
		.grid_line        = (Color){70, 100, 180, 70},
		.grid_bright      = (Color){150, 200, 255, 120},
		.grid_vert        = (Color){60, 110, 200, 70},
		.caption          = (Color){150, 220, 255, 255},
		.timeline_title   = (Color){180, 230, 255, 255},
		.speed_running    = (Color){180, 230, 255, 255},
		.scen_counter     = (Color){160, 200, 240, 255},
		.sublabel         = (Color){145, 185, 225, 255},
		.key              = (Color){255, 255, 255, 255},
		.key_desc         = (Color){170, 205, 240, 255},
		.meter_label      = (Color){170, 220, 255, 255},
		.tick_counter     = (Color){255, 255, 255, 255},
		.badge_text       = (Color){140, 180, 220, 255},
		.axis_label       = (Color){235, 235, 255, 255},
		.fault_pct_text   = (Color){255, 255, 255, 255},
		.primary_text     = (Color){220, 230, 255, 255},
		.knob_fill        = (Color){255, 255, 255, 255},
		.mode_default     = (Color){220, 230, 255, 255},
		.brak             = (Color){90, 170, 220, 255},
		.ok               = (Color){85, 255, 85, 255},
		.warning          = (Color){255, 220, 90, 255},
		.shutdown         = (Color){255, 96, 96, 255},
		.warning_fill     = (Color){255, 220, 90, 200},
		.shutdown_fill    = (Color){255, 96, 96, 210},
		.warning_dash     = (Color){255, 220, 90, 170},
		.shutdown_dash    = (Color){255, 96, 96, 170},
		.oil_shut_dash    = (Color){255, 96, 96, 140},
		.warning_tint     = (Color){255, 196, 64, 72},
		.shutdown_tint    = (Color){255, 64, 64, 88},
		.speed_paused     = (Color){255, 220, 90, 220},
		.end_notice       = (Color){255, 240, 120, 255},
		.playhead         = (Color){255, 255, 255, 220},
		.rpm              = (Color){255, 170, 170, 255},
		.temp             = (Color){255, 220, 90, 255},
		.oil              = (Color){160, 200, 255, 255},
		.ctrl             = (Color){85, 255, 255, 255},
		.slider_fill      = (Color){85, 255, 255, 200},
		.slider_ring      = (Color){170, 200, 240, 255},
		.replay_hint      = (Color){170, 200, 240, 255},
	},
	[VISUALIZER_THEME_ONYX] = {
		.label            = "ONYX",
		.window_bg        = (Color){10, 12, 16, 255},
		.grad_top         = (Color){18, 20, 26, 255},
		.grad_bot         = (Color){6, 8, 12, 255},
		.panel_bg         = (Color){16, 19, 25, 255},
		.nav_bg           = (Color){13, 15, 21, 255},
		.timeline_bg      = (Color){14, 17, 23, 255},
		.bar_bg           = (Color){24, 28, 38, 255},
		.badge_bg         = (Color){21, 24, 33, 255},
		.slider_track_bg  = (Color){26, 30, 42, 255},
		.knob_shadow      = (Color){0, 0, 0, 140},
		.hdr_border       = (Color){58, 63, 78, 255},
		.panel_border     = (Color){52, 58, 72, 255},
		.timeline_border  = (Color){58, 64, 80, 255},
		.section_div      = (Color){40, 46, 60, 255},
		.timeline_subdiv  = (Color){44, 50, 66, 255},
		.fault_bar_bor    = (Color){56, 61, 78, 255},
		.bar_border       = (Color){60, 66, 84, 255},
		.slider_track_bor = (Color){72, 80, 100, 255},
		.badge_border     = (Color){78, 88, 110, 255},
		.grid_line        = (Color){64, 71, 88, 110},
		.grid_bright      = (Color){140, 150, 172, 170},
		.grid_vert        = (Color){54, 60, 78, 90},
		.caption          = (Color){176, 156, 112, 255},
		.timeline_title   = (Color){190, 198, 214, 255},
		.speed_running    = (Color){90, 200, 188, 255},
		.scen_counter     = (Color){142, 150, 168, 255},
		.sublabel         = (Color){128, 136, 156, 255},
		.key              = (Color){118, 214, 202, 255},
		.key_desc         = (Color){124, 132, 150, 255},
		.meter_label      = (Color){150, 158, 176, 255},
		.tick_counter     = (Color){198, 206, 224, 255},
		.badge_text       = (Color){214, 220, 236, 255},
		.axis_label       = (Color){132, 140, 158, 255},
		.fault_pct_text   = (Color){196, 204, 220, 255},
		.primary_text     = (Color){245, 247, 250, 255},
		.knob_fill        = (Color){238, 242, 248, 255},
		.mode_default     = (Color){190, 198, 214, 255},
		.brak             = (Color){98, 106, 126, 255},
		.ok               = (Color){58, 199, 132, 255},
		.warning          = (Color){246, 189, 96, 255},
		.shutdown         = (Color){219, 78, 87, 255},
		.warning_fill     = (Color){246, 189, 96, 190},
		.shutdown_fill    = (Color){219, 78, 87, 200},
		.warning_dash     = (Color){246, 189, 96, 135},
		.shutdown_dash    = (Color){219, 78, 87, 145},
		.oil_shut_dash    = (Color){219, 78, 87, 95},
		.warning_tint     = (Color){246, 189, 96, 18},
		.shutdown_tint    = (Color){219, 78, 87, 24},
		.speed_paused     = (Color){246, 189, 96, 220},
		.end_notice       = (Color){236, 220, 174, 255},
		.playhead         = (Color){255, 255, 255, 220},
		.rpm              = (Color){79, 179, 255, 255},
		.temp             = (Color){255, 122, 122, 255},
		.oil              = (Color){78, 204, 143, 255},
		.ctrl             = (Color){94, 225, 205, 255},
		.slider_fill      = (Color){118, 214, 202, 255},
		.slider_ring      = (Color){118, 214, 202, 180},
		.replay_hint      = (Color){154, 162, 182, 255},
	},
	[VISUALIZER_THEME_GRUVBOX_DARK] = {
		.label            = "GRUVBOX",
		.window_bg        = (Color){29, 32, 33, 255},
		.grad_top         = (Color){40, 40, 40, 255},
		.grad_bot         = (Color){24, 26, 27, 255},
		.panel_bg         = (Color){40, 40, 40, 255},
		.nav_bg           = (Color){50, 48, 47, 255},
		.timeline_bg      = (Color){50, 48, 47, 255},
		.bar_bg           = (Color){60, 56, 54, 255},
		.badge_bg         = (Color){80, 73, 69, 255},
		.slider_track_bg  = (Color){60, 56, 54, 255},
		.knob_shadow      = (Color){0, 0, 0, 150},
		.hdr_border       = (Color){102, 92, 84, 255},
		.panel_border     = (Color){102, 92, 84, 255},
		.timeline_border  = (Color){124, 111, 100, 255},
		.section_div      = (Color){80, 73, 69, 255},
		.timeline_subdiv  = (Color){102, 92, 84, 255},
		.fault_bar_bor    = (Color){102, 92, 84, 255},
		.bar_border       = (Color){124, 111, 100, 255},
		.slider_track_bor = (Color){124, 111, 100, 255},
		.badge_border     = (Color){146, 131, 116, 255},
		.grid_line        = (Color){124, 111, 100, 120},
		.grid_bright      = (Color){168, 153, 132, 190},
		.grid_vert        = (Color){102, 92, 84, 110},
		.caption          = (Color){168, 153, 132, 255},
		.timeline_title   = (Color){235, 219, 178, 255},
		.speed_running    = (Color){142, 192, 124, 255},
		.scen_counter     = (Color){213, 196, 161, 255},
		.sublabel         = (Color){189, 174, 147, 255},
		.key              = (Color){131, 165, 152, 255},
		.key_desc         = (Color){168, 153, 132, 255},
		.meter_label      = (Color){213, 196, 161, 255},
		.tick_counter     = (Color){251, 241, 199, 255},
		.badge_text       = (Color){235, 219, 178, 255},
		.axis_label       = (Color){213, 196, 161, 255},
		.fault_pct_text   = (Color){251, 241, 199, 255},
		.primary_text     = (Color){251, 241, 199, 255},
		.knob_fill        = (Color){251, 241, 199, 255},
		.mode_default     = (Color){213, 196, 161, 255},
		.brak             = (Color){146, 131, 116, 255},
		.ok               = (Color){184, 187, 38, 255},
		.warning          = (Color){250, 189, 47, 255},
		.shutdown         = (Color){251, 73, 52, 255},
		.warning_fill     = (Color){250, 189, 47, 190},
		.shutdown_fill    = (Color){251, 73, 52, 205},
		.warning_dash     = (Color){250, 189, 47, 160},
		.shutdown_dash    = (Color){251, 73, 52, 170},
		.oil_shut_dash    = (Color){254, 128, 25, 145},
		.warning_tint     = (Color){250, 189, 47, 24},
		.shutdown_tint    = (Color){251, 73, 52, 28},
		.speed_paused     = (Color){250, 189, 47, 220},
		.end_notice       = (Color){254, 128, 25, 255},
		.playhead         = (Color){251, 241, 199, 220},
		.rpm              = (Color){131, 165, 152, 255},
		.temp             = (Color){211, 134, 155, 255},
		.oil              = (Color){142, 192, 124, 255},
		.ctrl             = (Color){254, 128, 25, 255},
		.slider_fill      = (Color){131, 165, 152, 255},
		.slider_ring      = (Color){131, 165, 152, 190},
		.replay_hint      = (Color){189, 174, 147, 255},
	},
	[VISUALIZER_THEME_LIGHT] = {
		.label            = "LIGHT",
		.window_bg        = (Color){244, 247, 250, 255},
		.grad_top         = (Color){252, 253, 255, 255},
		.grad_bot         = (Color){232, 237, 243, 255},
		.panel_bg         = (Color){250, 252, 255, 255},
		.nav_bg           = (Color){240, 244, 249, 255},
		.timeline_bg      = (Color){248, 250, 253, 255},
		.bar_bg           = (Color){226, 232, 240, 255},
		.badge_bg         = (Color){236, 241, 247, 255},
		.slider_track_bg  = (Color){216, 224, 234, 255},
		.knob_shadow      = (Color){80, 98, 120, 50},
		.hdr_border       = (Color){173, 183, 198, 255},
		.panel_border     = (Color){180, 190, 204, 255},
		.timeline_border  = (Color){173, 184, 198, 255},
		.section_div      = (Color){194, 202, 214, 255},
		.timeline_subdiv  = (Color){198, 206, 218, 255},
		.fault_bar_bor    = (Color){168, 179, 193, 255},
		.bar_border       = (Color){160, 171, 186, 255},
		.slider_track_bor = (Color){150, 163, 180, 255},
		.badge_border     = (Color){145, 158, 176, 255},
		.grid_line        = (Color){182, 191, 204, 130},
		.grid_bright      = (Color){108, 121, 141, 200},
		.grid_vert        = (Color){190, 198, 210, 120},
		.caption          = (Color){86, 97, 114, 255},
		.timeline_title   = (Color){70, 80, 98, 255},
		.speed_running    = (Color){39, 121, 118, 255},
		.scen_counter     = (Color){92, 102, 120, 255},
		.sublabel         = (Color){95, 106, 124, 255},
		.key              = (Color){36, 118, 180, 255},
		.key_desc         = (Color){90, 101, 120, 255},
		.meter_label      = (Color){66, 77, 94, 255},
		.tick_counter     = (Color){62, 73, 91, 255},
		.badge_text       = (Color){66, 77, 94, 255},
		.axis_label       = (Color){84, 95, 114, 255},
		.fault_pct_text   = (Color){70, 81, 96, 255},
		.primary_text     = (Color){32, 43, 56, 255},
		.knob_fill        = (Color){252, 253, 255, 255},
		.mode_default     = (Color){85, 97, 114, 255},
		.brak             = (Color){118, 129, 146, 255},
		.ok               = (Color){34, 146, 88, 255},
		.warning          = (Color){255, 193, 7, 255},
		.shutdown         = (Color){219, 78, 87, 255},
		.warning_fill     = (Color){190, 132, 20, 180},
		.shutdown_fill    = (Color){190, 60, 76, 190},
		.warning_dash     = (Color){190, 132, 20, 140},
		.shutdown_dash    = (Color){190, 60, 76, 150},
		.oil_shut_dash    = (Color){190, 60, 76, 110},
		.warning_tint     = (Color){190, 132, 20, 18},
		.shutdown_tint    = (Color){190, 60, 76, 22},
		.speed_paused     = (Color){176, 120, 24, 255},
		.end_notice       = (Color){129, 99, 23, 255},
		.playhead         = (Color){46, 59, 79, 220},
		.rpm              = (Color){38, 132, 214, 255},
		.temp             = (Color){230, 96, 114, 255},
		.oil              = (Color){42, 162, 92, 255},
		.ctrl             = (Color){89, 106, 214, 255},
		.slider_fill      = (Color){49, 124, 199, 255},
		.slider_ring      = (Color){49, 124, 199, 170},
		.replay_hint      = (Color){92, 103, 121, 255},
	},
};

static VisualizerThemeId g_visualizer_theme = VISUALIZER_THEME_DEFAULT;

const float FS_TINY                          = 14.0f;
const float FS_SMALL                         = 14.0f;
const float FS_KEY_HINT                      = 14.0f;
const float FS_VALUE                         = 14.0f;
const float FS_BADGE                         = 14.0f;
const float FS_MODE                          = 28.0f;
const float FS_SCEN_NAME                     = 14.0f;
const float FS_TL_TITLE                      = 14.0f;
const float FS_TL_LEGEND                     = 14.0f;
const float FS_TL_AXIS                       = 14.0f;

const float LAYOUT_PAD                       = 16.0f;
const float LAYOUT_HDR_H                     = 50.0f;
const float LAYOUT_NAV_H                     = 30.0f;
const float LAYOUT_MAIN_H                    = 252.0f;
const float LAYOUT_STATUS_W                  = 310.0f;
const float LAYOUT_COL_GAP                   = 12.0f;
const float LAYOUT_BAR_H                     = 22.0f;
const float LAYOUT_ROW_STEP                  = 42.0f;
const float LAYOUT_BADGE_H                   = 28.0f;
const float LAYOUT_KNOB_R                    = 9.0f;
const float LAYOUT_KNOB_SHADOW               = 11.0f;
const float LAYOUT_DOT_R                     = 5.0f;
const float LAYOUT_LEGEND_DOT_GAP            = 6.0f;
const float LAYOUT_LEGEND_COL_GAP            = 18.0f;
const float LAYOUT_SLIDER_INSET              = 40.0f;
const float LAYOUT_SLIDER_H                  = 24.0f;
const float LAYOUT_HEADER_TOP_Y              = 10.0f;
const float LAYOUT_HEADER_MAIN_Y             = 26.0f;
const float LAYOUT_HEADER_SEGMENT_GAP        = 24.0f;
const float LAYOUT_HEADER_META_GAP           = 18.0f;

const float TL_HEADER_H                      = 26.0f;
const float TL_TITLE_X                       = 16.0f;
const float TL_TITLE_Y                       = 6.0f;
const float LAYOUT_TIMELINE_PLOT_LEFT        = 52.0f;
const float LAYOUT_TIMELINE_PLOT_RIGHT       = 18.0f;
const float LAYOUT_TIMELINE_PLOT_TOP         = 40.0f;
const float LAYOUT_TIMELINE_AXIS_X           = 10.0f;
const float LAYOUT_TIMELINE_AXIS_Y_NUDGE     = 7.0f;
const float LAYOUT_TIMELINE_XLABEL_Y         = 6.0f;
const float LAYOUT_TIMELINE_XLABEL_BAND      = 20.0f;
const float LAYOUT_TIMELINE_LEGEND_RIGHT     = 16.0f;
const float LAYOUT_TIMELINE_LEGEND_Y         = 6.0f;
const float LAYOUT_TIMELINE_SLIDER_BOTTOM_PAD = 8.0f;

const float LAYOUT_STATUS_MODE_X             = 14.0f;
const float LAYOUT_STATUS_RESULT_X_PCT       = 0.33f;
const float LAYOUT_STATUS_RUN_RIGHT          = 120.0f;
const float LAYOUT_STATUS_CAP_Y              = 40.0f;
const float LAYOUT_STATUS_VALUE_GAP          = 18.0f;
const float LAYOUT_STATUS_FAULT_DIV_Y        = 126.0f;
const float LAYOUT_STATUS_FAULT_TITLE_Y      = 136.0f;
const float LAYOUT_STATUS_FAULT_ROW0_Y       = 160.0f;
const float LAYOUT_STATUS_FAULT_ROW1_Y       = 176.0f;
const float LAYOUT_STATUS_FAULT_LABEL_GAP    = 10.0f;
const float LAYOUT_STATUS_END_DIV_Y          = 196.0f;
const float LAYOUT_STATUS_END_TITLE_Y        = 206.0f;
const float LAYOUT_STATUS_END_HINT_Y         = 222.0f;

static int theme_id_is_valid(VisualizerThemeId theme_id)
{
	return (theme_id >= 0) && (theme_id < VISUALIZER_THEME_COUNT);
}

static int theme_name_matches(const char *name, const char *candidate)
{
	return strcmp(name, candidate) == 0;
}

const VisualizerTheme *visualizer_active_theme(void)
{
	return &k_visualizer_themes[g_visualizer_theme];
}

int visualizer_parse_theme_id(const char *name, VisualizerThemeId *theme_id)
{
	if ((name == NULL) || (theme_id == NULL)) {
		return 0;
	}

	if (theme_name_matches(name, "default")) {
		*theme_id = VISUALIZER_THEME_DEFAULT;
		return 1;
	}

	if (theme_name_matches(name, "dos") ||
	    theme_name_matches(name, "dos-blue")) {
		*theme_id = VISUALIZER_THEME_DOS_BLUE;
		return 1;
	}

	if (theme_name_matches(name, "onyx")) {
		*theme_id = VISUALIZER_THEME_ONYX;
		return 1;
	}

	if (theme_name_matches(name, "gruvbox")) {
		*theme_id = VISUALIZER_THEME_GRUVBOX_DARK;
		return 1;
	}

	if (theme_name_matches(name, "light")) {
		*theme_id = VISUALIZER_THEME_LIGHT;
		return 1;
	}

	return 0;
}

const char *visualizer_theme_label(VisualizerThemeId theme_id)
{
	if (!theme_id_is_valid(theme_id)) {
		return k_visualizer_themes[VISUALIZER_THEME_DEFAULT].label;
	}

	return k_visualizer_themes[theme_id].label;
}

VisualizerThemeId visualizer_theme_get(void)
{
	return g_visualizer_theme;
}

VisualizerThemeId visualizer_theme_cycle(void)
{
	g_visualizer_theme =
		(VisualizerThemeId)((g_visualizer_theme + 1) %
					VISUALIZER_THEME_COUNT);
	return g_visualizer_theme;
}

void visualizer_theme_set(VisualizerThemeId theme_id)
{
	if (!theme_id_is_valid(theme_id)) {
		g_visualizer_theme = VISUALIZER_THEME_DEFAULT;
		return;
	}

	g_visualizer_theme = theme_id;
}
