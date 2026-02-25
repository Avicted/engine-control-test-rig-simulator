#ifndef SCENARIO_PROFILES_H
#define SCENARIO_PROFILES_H

#include <stdint.h>

#include "engine.h"

typedef struct
{
    uint32_t tick;
    float rpm;
    float temp;
    float oil;
    int32_t run;
    int32_t result;
    float control;
    EngineStateMode mode;
} TickReport;

int32_t execute_profile(EngineState *engine,
                        const uint32_t *tick_values,
                        const float *rpm_values,
                        const float *temp_values,
                        const float *oil_values,
                        const int32_t *run_values,
                        uint32_t tick_count,
                        int32_t show_sim,
                        int32_t show_control,
                        int32_t show_state,
                        TickReport *tick_reports,
                        uint32_t tick_report_capacity,
                        uint32_t *tick_report_count);

int32_t scenario_normal_operation(EngineState *engine,
                                  int32_t show_sim,
                                  int32_t show_control,
                                  int32_t show_state,
                                  void *tick_reports,
                                  uint32_t tick_report_capacity,
                                  uint32_t *tick_report_count);
int32_t scenario_overheat_short_duration(EngineState *engine,
                                         int32_t show_sim,
                                         int32_t show_control,
                                         int32_t show_state,
                                         void *tick_reports,
                                         uint32_t tick_report_capacity,
                                         uint32_t *tick_report_count);
int32_t scenario_overheat_persistent(EngineState *engine,
                                     int32_t show_sim,
                                     int32_t show_control,
                                     int32_t show_state,
                                     void *tick_reports,
                                     uint32_t tick_report_capacity,
                                     uint32_t *tick_report_count);
int32_t scenario_oil_pressure_persistent(EngineState *engine,
                                         int32_t show_sim,
                                         int32_t show_control,
                                         int32_t show_state,
                                         void *tick_reports,
                                         uint32_t tick_report_capacity,
                                         uint32_t *tick_report_count);
int32_t scenario_combined_warning_persistent(EngineState *engine,
                                             int32_t show_sim,
                                             int32_t show_control,
                                             int32_t show_state,
                                             void *tick_reports,
                                             uint32_t tick_report_capacity,
                                             uint32_t *tick_report_count);
int32_t scenario_cold_start_warmup_and_ramp(EngineState *engine,
                                            int32_t show_sim,
                                            int32_t show_control,
                                            int32_t show_state,
                                            void *tick_reports,
                                            uint32_t tick_report_capacity,
                                            uint32_t *tick_report_count);
int32_t scenario_high_load_warning_then_recovery(EngineState *engine,
                                                 int32_t show_sim,
                                                 int32_t show_control,
                                                 int32_t show_state,
                                                 void *tick_reports,
                                                 uint32_t tick_report_capacity,
                                                 uint32_t *tick_report_count);
int32_t scenario_oil_pressure_gradual_drain(EngineState *engine,
                                            int32_t show_sim,
                                            int32_t show_control,
                                            int32_t show_state,
                                            void *tick_reports,
                                            uint32_t tick_report_capacity,
                                            uint32_t *tick_report_count);
int32_t scenario_thermal_runaway_with_load_surge(EngineState *engine,
                                                 int32_t show_sim,
                                                 int32_t show_control,
                                                 int32_t show_state,
                                                 void *tick_reports,
                                                 uint32_t tick_report_capacity,
                                                 uint32_t *tick_report_count);
int32_t scenario_intermittent_oil_then_combined_fault(EngineState *engine,
                                                      int32_t show_sim,
                                                      int32_t show_control,
                                                      int32_t show_state,
                                                      void *tick_reports,
                                                      uint32_t tick_report_capacity,
                                                      uint32_t *tick_report_count);

#endif
