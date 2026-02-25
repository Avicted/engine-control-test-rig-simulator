/**
 * @file control.h
 * @brief Safety evaluation and calibration management for engine fault detection.
 */

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

/**
 * @brief Evaluate engine state against calibrated fault thresholds.
 *
 * Increments persistence counters for temperature, oil pressure, and
 * combined RPM+temperature faults. Triggers state transitions (WARNING,
 * SHUTDOWN) when persistence windows are met.
 *
 * @requirement REQ-ENG-001
 * @pre engine != NULL, evaluation_result != NULL
 * @post fault persistence counters and mode are deterministically updated
 * @deterministic yes
 *
 * @param[in,out] engine         Engine state with fault counters to update.
 * @param[out]    evaluation_result  Receives ENGINE_OK, ENGINE_WARNING, or ENGINE_SHUTDOWN.
 * @retval STATUS_OK              Evaluation completed successfully.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer argument.
 * @retval STATUS_INTERNAL_ERROR   Illegal state transition detected.
 */
StatusCode evaluate_engine(EngineState *engine, int32_t *evaluation_result);

/**
 * @brief Compute control output percentage from engine state.
 *
 * Output is clamped to [0.0, 100.0] based on RPM, temperature, and oil pressure.
 *
 * @requirement REQ-ENG-003
 * @pre engine != NULL, control_output != NULL
 * @post *control_output is clamped to [0, 100]
 * @deterministic yes
 *
 * @param[in]  engine          Current engine state.
 * @param[out] control_output  Receives clamped control output percentage.
 * @retval STATUS_OK              Computation completed successfully.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer argument.
 */
StatusCode compute_control_output(const EngineState *engine, float *control_output);

/**
 * @brief Retrieve factory-default calibration values.
 * @param[out] calibration_out  Receives default calibration.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode control_get_default_calibration(ControlCalibration *calibration_out);

/**
 * @brief Retrieve the currently active calibration.
 * @param[out] calibration_out  Receives active calibration snapshot.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode control_get_active_calibration(ControlCalibration *calibration_out);

/**
 * @brief Apply a validated calibration. May only be called once before reset.
 * @param[in] calibration  Calibration to apply (validated by calibration_valid()).
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT Invalid calibration or already configured.
 */
StatusCode control_configure_calibration(const ControlCalibration *calibration);

/**
 * @brief Reset calibration state to factory defaults.
 *
 * Clears the configured flag, allowing a new call to
 * control_configure_calibration().
 *
 * @requirement REQ-ENG-CAL-001
 * @pre none
 * @post calibration state is reset to defaults, allowing reconfiguration
 * @deterministic yes
 *
 * @retval STATUS_OK Always succeeds.
 */
StatusCode control_reset_calibration(void);

#endif
