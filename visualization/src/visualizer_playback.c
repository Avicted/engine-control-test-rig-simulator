#include <stdio.h>
#include <string.h>

#include "visualizer_playback.h"

static int string_matches(const char *value, const char *expected)
{
    return (value != NULL) && (strcmp(value, expected) == 0);
}

static void copy_tick_text(char *dst, size_t dst_size, const char *src)
{
    if ((dst == NULL) || (dst_size == 0U)) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    (void)snprintf(dst, dst_size, "%s", src);
}

SeverityLevel visualizer_mode_to_level(const char *mode, const char *result)
{
    if (string_matches(mode, "SHUTDOWN") || string_matches(result, "SHUTDOWN")) {
        return LEVEL_SHUTDOWN;
    }

    if (string_matches(mode, "WARNING") || string_matches(result, "WARNING")) {
        return LEVEL_WARNING;
    }

    return LEVEL_OK;
}

SeverityLevel visualizer_temp_to_level(float temp)
{
    if (temp > TEMP_SHUTDOWN_THRESHOLD) {
        return LEVEL_SHUTDOWN;
    }
    if (temp >= TEMP_WARNING_THRESHOLD) {
        return LEVEL_WARNING;
    }
    return LEVEL_OK;
}

SeverityLevel visualizer_oil_to_level(float oil)
{
    if (oil < OIL_SHUTDOWN_THRESHOLD) {
        return LEVEL_SHUTDOWN;
    }
    return LEVEL_OK;
}

SeverityLevel visualizer_rpm_to_level(float rpm, float temp)
{
    if ((rpm >= RPM_WARNING_THRESHOLD) && (temp >= TEMP_WARNING_THRESHOLD)) {
        return LEVEL_WARNING;
    }
    return LEVEL_OK;
}

float visualizer_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

float visualizer_default_ticks_per_second(const ScenarioData *scenario)
{
    if ((scenario != NULL) && (scenario->tick_count <= 8U)) {
        return SHORT_SCENARIO_TICKS_PER_SECOND;
    }

    return DEFAULT_TICKS_PER_SECOND;
}

static float lerp_float(float start_value, float end_value, float t)
{
    return start_value + ((end_value - start_value) * visualizer_clamp01(t));
}

void visualizer_interpolate_tick(const ScenarioData *scenario, float playhead, TickData *out_tick)
{
    unsigned int base_index = 0U;
    unsigned int next_index = 0U;
    float phase = 0.0f;

    if ((scenario == NULL) || (out_tick == NULL) || (scenario->tick_count == 0U)) {
        return;
    }

    if (playhead <= 0.0f) {
        *out_tick = scenario->ticks[0U];
        return;
    }

    if (playhead >= (float)(scenario->tick_count - 1U)) {
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

    if (phase >= 0.5f) {
        const TickData *next_tick = &scenario->ticks[next_index];

        out_tick->tick = next_tick->tick;
        out_tick->run = next_tick->run;
        copy_tick_text(out_tick->result, sizeof(out_tick->result), next_tick->result);
        copy_tick_text(out_tick->engine_mode, sizeof(out_tick->engine_mode), next_tick->engine_mode);
    }
}

void visualizer_compute_cumulative_metrics(const ScenarioData *scenario,
                                           float playhead,
                                           float *warning_pct,
                                           float *shutdown_pct)
{
    unsigned int i = 0U;
    unsigned int warn_count = 0U;
    unsigned int shut_count = 0U;
    unsigned int active_count = 0U;

    if ((scenario == NULL) || (warning_pct == NULL) || (shutdown_pct == NULL) || (scenario->tick_count == 0U)) {
        return;
    }

    active_count = (unsigned int)(playhead + 0.5f) + 1U;
    if (active_count > scenario->tick_count) {
        active_count = scenario->tick_count;
    }

    for (i = 0U; i < active_count; ++i) {
        SeverityLevel level = visualizer_mode_to_level(scenario->ticks[i].engine_mode, scenario->ticks[i].result);
        if (level == LEVEL_WARNING) {
            warn_count++;
        } else if (level == LEVEL_SHUTDOWN) {
            shut_count++;
        }
    }

    *warning_pct = (100.0f * (float)warn_count) / (float)active_count;
    *shutdown_pct = (100.0f * (float)shut_count) / (float)active_count;
}
