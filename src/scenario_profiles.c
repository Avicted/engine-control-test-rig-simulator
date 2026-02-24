#include "scenario_profiles.h"

int32_t scenario_normal_operation(EngineState *engine,
                                  int32_t show_sim,
                                  int32_t show_control,
                                  int32_t show_state,
                                  void *tick_reports,
                                  uint32_t tick_report_capacity,
                                  uint32_t *tick_report_count)
{
    static const float rpm_values[] = {2200.0f, 2600.0f, 3000.0f, 3200.0f};
    static const float temp_values[] = {76.0f, 80.0f, 83.0f, 84.5f};
    static const float oil_values[] = {3.2f, 3.1f, 3.0f, 2.9f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_overheat_short_duration(EngineState *engine,
                                         int32_t show_sim,
                                         int32_t show_control,
                                         int32_t show_state,
                                         void *tick_reports,
                                         uint32_t tick_report_capacity,
                                         uint32_t *tick_report_count)
{
    static const float rpm_values[] = {2400.0f, 2400.0f, 2400.0f, 2400.0f};
    static const float temp_values[] = {96.0f, 97.0f, 90.0f, 89.0f};
    static const float oil_values[] = {3.1f, 3.1f, 3.1f, 3.1f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_overheat_persistent(EngineState *engine,
                                     int32_t show_sim,
                                     int32_t show_control,
                                     int32_t show_state,
                                     void *tick_reports,
                                     uint32_t tick_report_capacity,
                                     uint32_t *tick_report_count)
{
    static const float rpm_values[] = {2500.0f, 2500.0f, 2500.0f, 2500.0f};
    static const float temp_values[] = {96.0f, 97.0f, 98.0f, 99.0f};
    static const float oil_values[] = {3.0f, 3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_oil_pressure_persistent(EngineState *engine,
                                         int32_t show_sim,
                                         int32_t show_control,
                                         int32_t show_state,
                                         void *tick_reports,
                                         uint32_t tick_report_capacity,
                                         uint32_t *tick_report_count)
{
    static const float rpm_values[] = {2600.0f, 2600.0f, 2600.0f, 2600.0f};
    static const float temp_values[] = {82.0f, 82.0f, 82.0f, 82.0f};
    static const float oil_values[] = {2.4f, 2.3f, 2.2f, 2.1f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_combined_warning_persistent(EngineState *engine,
                                             int32_t show_sim,
                                             int32_t show_control,
                                             int32_t show_state,
                                             void *tick_reports,
                                             uint32_t tick_report_capacity,
                                             uint32_t *tick_report_count)
{
    static const float rpm_values[] = {3600.0f, 3600.0f, 3300.0f};
    static const float temp_values[] = {86.0f, 87.0f, 80.0f};
    static const float oil_values[] = {3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_cold_start_warmup_and_ramp(EngineState *engine,
                                            int32_t show_sim,
                                            int32_t show_control,
                                            int32_t show_state,
                                            void *tick_reports,
                                            uint32_t tick_report_capacity,
                                            uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        800.0f, 1100.0f, 1400.0f, 1700.0f, 2000.0f,
        2200.0f, 2400.0f, 2600.0f, 2800.0f, 3000.0f,
        3100.0f, 3200.0f, 3300.0f, 3400.0f, 3400.0f,
        3400.0f, 3400.0f, 3300.0f, 3100.0f, 2900.0f,
        2700.0f, 2400.0f, 2100.0f, 1700.0f, 1300.0f};
    static const float temp_values[] = {
        44.0f, 49.0f, 54.0f, 58.5f, 62.0f,
        65.5f, 68.0f, 70.5f, 73.0f, 75.0f,
        77.0f, 78.5f, 80.0f, 81.0f, 82.0f,
        82.5f, 82.0f, 81.0f, 80.0f, 78.5f,
        77.0f, 74.5f, 71.0f, 67.5f, 63.0f};
    static const float oil_values[] = {
        3.8f, 3.7f, 3.7f, 3.6f, 3.5f,
        3.4f, 3.4f, 3.3f, 3.2f, 3.2f,
        3.1f, 3.1f, 3.0f, 3.0f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.1f,
        3.1f, 3.2f, 3.3f, 3.4f, 3.5f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_high_load_warning_then_recovery(EngineState *engine,
                                                 int32_t show_sim,
                                                 int32_t show_control,
                                                 int32_t show_state,
                                                 void *tick_reports,
                                                 uint32_t tick_report_capacity,
                                                 uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        2000.0f, 2400.0f, 2700.0f, 3000.0f, 3200.0f,
        3400.0f, 3500.0f, 3600.0f, 3700.0f, 3500.0f,
        3300.0f, 3100.0f, 2900.0f, 2700.0f, 2500.0f,
        2300.0f, 2100.0f, 2000.0f};
    static const float temp_values[] = {
        70.0f, 73.0f, 75.5f, 78.0f, 80.5f,
        82.5f, 85.0f, 87.0f, 88.0f, 87.0f,
        86.0f, 84.5f, 82.0f, 79.5f, 77.0f,
        74.5f, 72.0f, 70.0f};
    static const float oil_values[] = {
        3.2f, 3.2f, 3.1f, 3.1f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f,
        3.0f, 3.1f, 3.1f, 3.2f, 3.2f,
        3.2f, 3.3f, 3.3f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_oil_pressure_gradual_drain(EngineState *engine,
                                            int32_t show_sim,
                                            int32_t show_control,
                                            int32_t show_state,
                                            void *tick_reports,
                                            uint32_t tick_report_capacity,
                                            uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        2500.0f, 2700.0f, 2900.0f, 3000.0f, 3000.0f,
        3000.0f, 3000.0f, 3000.0f, 2900.0f, 2800.0f,
        2700.0f, 2600.0f, 2500.0f, 2500.0f, 2500.0f,
        2500.0f};
    static const float temp_values[] = {
        74.0f, 76.0f, 77.5f, 78.5f, 79.5f,
        80.0f, 80.0f, 80.5f, 80.0f, 79.5f,
        79.0f, 78.5f, 77.5f, 77.0f, 76.5f,
        76.0f};
    static const float oil_values[] = {
        3.5f, 3.4f, 3.3f, 3.2f, 3.1f,
        3.0f, 2.9f, 2.8f, 2.7f, 2.6f,
        2.5f, 2.4f, 2.3f, 2.2f, 2.1f,
        2.0f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_thermal_runaway_with_load_surge(EngineState *engine,
                                                 int32_t show_sim,
                                                 int32_t show_control,
                                                 int32_t show_state,
                                                 void *tick_reports,
                                                 uint32_t tick_report_capacity,
                                                 uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        2200.0f, 2500.0f, 2800.0f, 3000.0f, 3200.0f,
        3400.0f, 3400.0f, 3400.0f, 3400.0f, 3400.0f,
        3400.0f, 3400.0f, 3400.0f, 3400.0f, 3400.0f};
    static const float temp_values[] = {
        68.0f, 71.0f, 74.0f, 76.5f, 79.0f,
        81.5f, 84.0f, 87.5f, 90.5f, 93.0f,
        95.5f, 97.0f, 98.5f, 99.0f, 99.5f};
    static const float oil_values[] = {
        3.3f, 3.2f, 3.1f, 3.1f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}

int32_t scenario_intermittent_oil_then_combined_fault(EngineState *engine,
                                                      int32_t show_sim,
                                                      int32_t show_control,
                                                      int32_t show_state,
                                                      void *tick_reports,
                                                      uint32_t tick_report_capacity,
                                                      uint32_t *tick_report_count)
{
    static const float rpm_values[] = {
        2000.0f, 2200.0f, 2400.0f, 2500.0f, 2500.0f,
        2700.0f, 2900.0f, 3100.0f, 3300.0f, 3500.0f,
        3600.0f, 3500.0f, 3200.0f, 3000.0f, 2800.0f,
        2600.0f};
    static const float temp_values[] = {
        70.0f, 72.0f, 74.0f, 76.0f, 76.0f,
        78.0f, 80.0f, 82.0f, 83.0f, 85.0f,
        86.0f, 87.0f, 85.0f, 83.0f, 80.0f,
        77.0f};
    static const float oil_values[] = {
        3.4f, 3.3f, 2.4f, 2.3f, 2.6f,
        2.8f, 3.0f, 3.0f, 3.0f, 3.0f,
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f,
        3.1f};

    return execute_profile(engine,
                           (const uint32_t *)0,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (const int32_t *)0,
                           (uint32_t)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state,
                           (TickReport *)tick_reports,
                           tick_report_capacity,
                           tick_report_count);
}
