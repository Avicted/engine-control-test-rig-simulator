#ifndef VISUALIZER_UI_H
#define VISUALIZER_UI_H

#include "raylib.h"

#include "visualizer_model.h"

typedef struct
{
    Rectangle gauges_panel;
    Rectangle metrics_area;
    Rectangle rpm_bar;
    Rectangle rpm_val;
    Rectangle temp_bar;
    Rectangle temp_val;
    Rectangle oil_bar;
    Rectangle oil_val;
    Rectangle timeline;
    Rectangle slider;
    float scale;
    float pad;
    float hdr_h;
    float nav_h;
} VisualizerLayout;

Color visualizer_mode_color(const char *engine_mode);
Color visualizer_lerp_color(Color a, Color b, float t);
void visualizer_compute_layout(int screen_w, int screen_h, VisualizerLayout *layout);
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
                           int quit_modal_selection);

#endif