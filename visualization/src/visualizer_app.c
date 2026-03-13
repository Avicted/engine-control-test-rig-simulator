#include <stddef.h>
#include <math.h>

#include "raylib.h"

#include "visualizer_app.h"
#include "visualizer_playback.h"
#include "visualizer_ui.h"

#define KEY_SCRUB_DEBOUNCE_SECONDS 0.5f
#define KEY_SCRUB_TICKS_PER_SECOND 14.0f
#define KEY_SCRUB_ACCEL_TICKS_PER_SECOND 18.0f
#define KEY_SCRUB_MAX_TICKS_PER_SECOND 42.0f

typedef struct {
    float playhead;
    float ticks_per_second;
    float restart_feedback_timer;
    float key_scrub_hold_timer;
    int paused;
    int dragging_slider;
    int speed_overridden;
    int key_scrub_direction;
    int quit_modal_open;
    int quit_modal_selection;
    int should_quit;
    Color animated_mode_color;
    Font ui_font;
    int custom_font_loaded;
} VisualizerAppState;

static void initialize_ui_font(VisualizerAppState *state)
{
    state->ui_font = LoadFontEx(FONT_PATH, 14, NULL, 0);
    if ((state->ui_font.texture.id > 0U) && (state->ui_font.glyphCount > 0)) {
        SetTextureFilter(state->ui_font.texture, TEXTURE_FILTER_POINT);
        state->custom_font_loaded = 1;
    } else {
        state->ui_font = GetFontDefault();
        state->custom_font_loaded = 0;
    }
}

static void reset_for_active_scenario(const ScenarioSet *scenario_set, VisualizerAppState *state)
{
    const ScenarioData *scenario = &scenario_set->scenarios[scenario_set->active_index];

    state->playhead = 0.0f;
    state->paused = 0;
    state->dragging_slider = 0;
    state->restart_feedback_timer = 0.6f;
    state->key_scrub_hold_timer = 0.0f;
    state->key_scrub_direction = 0;
    if (state->speed_overridden == 0) {
        state->ticks_per_second = visualizer_default_ticks_per_second(scenario);
    }
    state->animated_mode_color = visualizer_mode_color(scenario->ticks[0U].engine_mode);
}

static void handle_window_commands(void)
{
    if (IsKeyPressed(KEY_F11)) {
        if (!IsWindowFullscreen()) {
            int monitor = GetCurrentMonitor();
            SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
            ToggleFullscreen();
        } else {
            ToggleFullscreen();
            SetWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        }
    }
}

static float current_key_scrub_speed(float hold_timer)
{
    float accelerated_hold_time = 0.0f;
    float scrub_speed = 0.0f;

    accelerated_hold_time = hold_timer - KEY_SCRUB_DEBOUNCE_SECONDS;
    if (accelerated_hold_time < 0.0f) {
        accelerated_hold_time = 0.0f;
    }

    scrub_speed = KEY_SCRUB_TICKS_PER_SECOND + (accelerated_hold_time * KEY_SCRUB_ACCEL_TICKS_PER_SECOND);
    if (scrub_speed > KEY_SCRUB_MAX_TICKS_PER_SECOND) {
        scrub_speed = KEY_SCRUB_MAX_TICKS_PER_SECOND;
    }

    return scrub_speed;
}

static void handle_quit_modal_input(VisualizerAppState *state)
{
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (state->quit_modal_open != 0) {
            state->quit_modal_open = 0;
            state->quit_modal_selection = 0;
        } else {
            state->quit_modal_open = 1;
            state->quit_modal_selection = 0;
        }
    }

    if (state->quit_modal_open == 0) {
        return;
    }

    if (IsKeyPressed(KEY_LEFT)) {
        state->quit_modal_selection = 1;
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        state->quit_modal_selection = 0;
    }
    if (IsKeyPressed(KEY_ENTER)) {
        if (state->quit_modal_selection == 1) {
            state->should_quit = 1;
        } else {
            state->quit_modal_open = 0;
            state->quit_modal_selection = 0;
        }
    }
}

static void handle_playback_input(ScenarioSet *scenario_set, VisualizerAppState *state, const VisualizerLayout *layout)
{
    ScenarioData *scenario = &scenario_set->scenarios[scenario_set->active_index];
    int right_pressed = IsKeyPressed(KEY_RIGHT);
    int left_pressed = IsKeyPressed(KEY_LEFT);
    int right_down = IsKeyDown(KEY_RIGHT);
    int left_down = IsKeyDown(KEY_LEFT);
    float max_playhead = (float)(scenario->tick_count - 1U);

    if (state->quit_modal_open != 0) {
        state->key_scrub_hold_timer = 0.0f;
        state->key_scrub_direction = 0;
        return;
    }

    if (IsKeyPressed(KEY_T)) {
        (void)visualizer_theme_cycle();
    }

    if (IsKeyPressed(KEY_SPACE)) {
        state->paused = (state->paused == 0) ? 1 : 0;
    }
    if (IsKeyPressed(KEY_R)) {
        state->playhead = 0.0f;
        state->paused = 0;
        state->restart_feedback_timer = 0.6f;
    }
    if (right_pressed != 0) {
        state->playhead = ceilf(state->playhead + 0.0001f);
        if (state->playhead > max_playhead) {
            state->playhead = max_playhead;
        }
        state->paused = 1;
        state->key_scrub_hold_timer = 0.0f;
        state->key_scrub_direction = 1;
    } else if (right_down != 0) {
        if (state->key_scrub_direction != 1) {
            state->key_scrub_hold_timer = 0.0f;
            state->key_scrub_direction = 1;
        } else {
            state->key_scrub_hold_timer += GetFrameTime();
        }

        if (state->key_scrub_hold_timer >= KEY_SCRUB_DEBOUNCE_SECONDS) {
            state->playhead += GetFrameTime() * current_key_scrub_speed(state->key_scrub_hold_timer);
            if (state->playhead > max_playhead) {
                state->playhead = max_playhead;
            }
            state->paused = 1;
        }
    } else if (left_pressed != 0) {
        state->playhead = floorf(state->playhead - 0.0001f);
        if (state->playhead < 0.0f) {
            state->playhead = 0.0f;
        }
        state->paused = 1;
        state->key_scrub_hold_timer = 0.0f;
        state->key_scrub_direction = -1;
    } else if (left_down != 0) {
        if (state->key_scrub_direction != -1) {
            state->key_scrub_hold_timer = 0.0f;
            state->key_scrub_direction = -1;
        } else {
            state->key_scrub_hold_timer += GetFrameTime();
        }

        if (state->key_scrub_hold_timer >= KEY_SCRUB_DEBOUNCE_SECONDS) {
            state->playhead -= GetFrameTime() * current_key_scrub_speed(state->key_scrub_hold_timer);
            if (state->playhead < 0.0f) {
                state->playhead = 0.0f;
            }
            state->paused = 1;
        }
    } else {
        state->key_scrub_hold_timer = 0.0f;
        state->key_scrub_direction = 0;
    }
    if (IsKeyPressed(KEY_UP)) {
        state->ticks_per_second += 1.0f;
        if (state->ticks_per_second > 20.0f) {
            state->ticks_per_second = 20.0f;
        }
        state->speed_overridden = 1;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        state->ticks_per_second -= 1.0f;
        if (state->ticks_per_second < 1.0f) {
            state->ticks_per_second = 1.0f;
        }
        state->speed_overridden = 1;
    }
    if (IsKeyPressed(KEY_TAB) && (scenario_set->count > 1U)) {
        scenario_set->active_index = (scenario_set->active_index + 1U) % scenario_set->count;
        reset_for_active_scenario(scenario_set, state);
        scenario = &scenario_set->scenarios[scenario_set->active_index];
        (void)scenario;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), layout->slider)) {
        state->dragging_slider = 1;
        state->paused = 1;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->dragging_slider = 0;
    }
    if (state->dragging_slider != 0) {
        float rel = (GetMousePosition().x - layout->slider.x) / layout->slider.width;
        rel = visualizer_clamp01(rel);
        state->playhead = roundf(rel * (float)(scenario->tick_count - 1U));
    }
}

static void update_playback(const ScenarioData *scenario, VisualizerAppState *state)
{
    if ((state->paused == 0) && (state->dragging_slider == 0) &&
        (state->playhead < (float)(scenario->tick_count - 1U))) {
        state->playhead += GetFrameTime() * state->ticks_per_second;
        if (state->playhead > (float)(scenario->tick_count - 1U)) {
            state->playhead = (float)(scenario->tick_count - 1U);
        }
    }

    if (state->restart_feedback_timer > 0.0f) {
        state->restart_feedback_timer -= GetFrameTime();
        if (state->restart_feedback_timer < 0.0f) {
            state->restart_feedback_timer = 0.0f;
        }
    }
}

void visualizer_run(ScenarioSet *scenario_set, VisualizerThemeId initial_theme)
{
    VisualizerAppState state = {0};

    if ((scenario_set == NULL) || (scenario_set->count == 0U)) {
        return;
    }

    state.should_quit = 0;
    state.quit_modal_open = 0;
    state.quit_modal_selection = 0;
    state.custom_font_loaded = 0;
    state.key_scrub_hold_timer = 0.0f;
    state.key_scrub_direction = 0;
    state.speed_overridden = 0;

    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Engine Control Scenario Visualizer");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    visualizer_theme_set(initial_theme);
    initialize_ui_font(&state);
    reset_for_active_scenario(scenario_set, &state);

    while (!WindowShouldClose() && (state.should_quit == 0)) {
        ScenarioData *scenario = &scenario_set->scenarios[scenario_set->active_index];
        TickData interpolated_tick = {0};
        float warning_pct = 0.0f;
        float shutdown_pct = 0.0f;
        VisualizerLayout layout = {0};

        visualizer_compute_layout(GetScreenWidth(), GetScreenHeight(), &layout);
        handle_window_commands();
        handle_quit_modal_input(&state);
        handle_playback_input(scenario_set, &state, &layout);

        scenario = &scenario_set->scenarios[scenario_set->active_index];
        update_playback(scenario, &state);
        visualizer_interpolate_tick(scenario, state.playhead, &interpolated_tick);
        state.animated_mode_color = visualizer_lerp_color(state.animated_mode_color,
                                  visualizer_mode_color(interpolated_tick.engine_mode),
                                  GetFrameTime() * 8.0f);
        visualizer_compute_cumulative_metrics(scenario, state.playhead, &warning_pct, &shutdown_pct);

        visualizer_draw_frame(&state.ui_font,
                      scenario_set,
                      scenario,
                      &interpolated_tick,
                      &layout,
                      state.playhead,
                      state.ticks_per_second,
                      state.paused,
                      state.animated_mode_color,
                      state.restart_feedback_timer,
                      warning_pct,
                      shutdown_pct,
                      state.quit_modal_open,
                      state.quit_modal_selection);
    }

    if (state.custom_font_loaded != 0) {
        UnloadFont(state.ui_font);
    }
    CloseWindow();
}
