#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

#include "engine.h"

#define CONTROL_DEFAULT_TEMP_LIMIT 95.0f
#define CONTROL_DEFAULT_OIL_PRESSURE_LIMIT 2.5f
#define CONTROL_DEFAULT_HIGH_RPM_WARNING_THRESHOLD 3500.0f
#define CONTROL_DEFAULT_HIGH_TEMP_WARNING_THRESHOLD 85.0f
#define CONTROL_DEFAULT_TEMP_PERSISTENCE_TICKS 3U
#define CONTROL_DEFAULT_OIL_PERSISTENCE_TICKS 3U
#define CONTROL_DEFAULT_COMBINED_WARNING_PERSISTENCE_TICKS 2U

typedef struct
{
    float temperature_limit;
    float oil_pressure_limit;
    float high_rpm_warning_threshold;
    float high_temp_warning_threshold;
    uint32_t temp_persistence_ticks;
    uint32_t oil_persistence_ticks;
    uint32_t combined_warning_persistence_ticks;
} ControlCalibration;

/*
 * @requirement REQ-ENG-001
 * @pre engine != NULL, evaluation_result != NULL
 * @post fault persistence counters and mode are deterministically updated
 * @deterministic yes
 */
StatusCode evaluate_engine(EngineState *engine, int32_t *evaluation_result);

/*
 * @requirement REQ-ENG-003
 * @pre engine != NULL, control_output != NULL
 * @post *control_output is clamped to [0, 100]
 * @deterministic yes
 */
StatusCode compute_control_output(const EngineState *engine, float *control_output);

StatusCode control_get_default_calibration(ControlCalibration *calibration_out);
StatusCode control_get_active_calibration(ControlCalibration *calibration_out);
StatusCode control_configure_calibration(const ControlCalibration *calibration);

#endif
