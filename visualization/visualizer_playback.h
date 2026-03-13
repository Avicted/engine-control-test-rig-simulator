#ifndef VISUALIZER_PLAYBACK_H
#define VISUALIZER_PLAYBACK_H

#include "visualizer_model.h"

SeverityLevel visualizer_mode_to_level(const char *mode, const char *result);
SeverityLevel visualizer_temp_to_level(float temp);
SeverityLevel visualizer_oil_to_level(float oil);
SeverityLevel visualizer_rpm_to_level(float rpm, float temp);
float visualizer_clamp01(float value);
float visualizer_default_ticks_per_second(const ScenarioData *scenario);
void visualizer_interpolate_tick(const ScenarioData *scenario, float playhead, TickData *out_tick);
void visualizer_compute_cumulative_metrics(const ScenarioData *scenario,
                                           float playhead,
                                           float *warning_pct,
                                           float *shutdown_pct);

#endif