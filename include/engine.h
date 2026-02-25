/**
 * @file engine.h
 * @brief Engine state machine: states, transitions, and deterministic tick update.
 */

#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

#include "status.h"

#define ENGINE_OK 0
#define ENGINE_WARNING 1
#define ENGINE_SHUTDOWN 2
#define ENGINE_ERROR -1

typedef enum
{
    ENGINE_STATE_INIT,
    ENGINE_STATE_STARTING,
    ENGINE_STATE_RUNNING,
    ENGINE_STATE_WARNING,
    ENGINE_STATE_SHUTDOWN
} EngineStateMode;

typedef enum
{
    ENGINE_FAULT_TEMP = 0,
    ENGINE_FAULT_OIL_PRESSURE = 1,
    ENGINE_FAULT_RPM_TEMP_COMBINED = 2,
    ENGINE_FAULT_COUNTER_COUNT = 3
} EngineFaultCounter;

typedef struct
{
    float rpm;
    float temperature;
    float oil_pressure;
    int32_t is_running;
    EngineStateMode mode;
    uint32_t fault_counters[ENGINE_FAULT_COUNTER_COUNT];
} EngineState;

/** @brief Default steady-state target RPM. */
#define ENGINE_DEFAULT_TARGET_RPM 3000.0f
/** @brief Default steady-state target temperature (deg C). */
#define ENGINE_DEFAULT_TARGET_TEMP 90.0f
/** @brief Default steady-state target oil pressure (bar). */
#define ENGINE_DEFAULT_TARGET_OIL 3.4f
/** @brief Default RPM ramp rate per tick (RPM/tick). */
#define ENGINE_DEFAULT_RPM_RAMP_RATE 150.0f
/** @brief Default temperature ramp rate per tick (deg C/tick). */
#define ENGINE_DEFAULT_TEMP_RAMP_RATE 0.6f
/** @brief Default oil pressure decay rate per tick (bar/tick). */
#define ENGINE_DEFAULT_OIL_DECAY_RATE 0.01f

/**
 * @brief Configurable engine plant-model physics parameters.
 *
 * Separates the physical plant model (this) from the control/safety
 * thresholds (ControlCalibration). This mirrors real engine
 * simulation practice where plant and controller are independently tuned.
 */
typedef struct
{
    float target_rpm;          /**< Steady-state RPM target. */
    float target_temperature;  /**< Steady-state temperature target (deg C). */
    float target_oil_pressure; /**< Steady-state oil pressure target (bar). */
    float rpm_ramp_rate;       /**< RPM increase per tick (RPM/tick). */
    float temp_ramp_rate;      /**< Temperature increase per tick (deg C/tick). */
    float oil_decay_rate;      /**< Oil pressure decrease per tick (bar/tick). */
} EnginePhysicsConfig;

_Static_assert(sizeof(int32_t) == 4U, "int32_t must be 32-bit");
_Static_assert(sizeof(uint32_t) == 4U, "uint32_t must be 32-bit");
_Static_assert(ENGINE_STATE_INIT == 0, "EngineStateMode ordinal contract changed");
_Static_assert(ENGINE_STATE_SHUTDOWN == 4, "EngineStateMode ordinal contract changed");
_Static_assert(ENGINE_FAULT_COUNTER_COUNT == 3, "Unexpected fault counter count");
_Static_assert((sizeof(((EngineState){0}).fault_counters) / sizeof(uint32_t)) == ENGINE_FAULT_COUNTER_COUNT,
               "EngineState fault counter array size mismatch");

/**
 * @brief Initialize engine state to deterministic baseline values.
 * @param[out] engine  Engine state to initialize.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode engine_init(EngineState *engine);

/**
 * @brief Reset engine state (alias for engine_init).
 *
 * @requirement REQ-ENG-000
 * @pre engine != NULL
 * @post engine is reinitialized to deterministic baseline values
 * @deterministic yes
 *
 * @param[out] engine  Engine state to reset.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode engine_reset(EngineState *engine);

/**
 * @brief Start the engine from INIT state.
 *
 * @requirement REQ-ENG-000
 * @pre engine != NULL and engine->mode == ENGINE_STATE_INIT
 * @post engine transitions to STARTING and run flag is enabled
 * @deterministic yes
 *
 * @param[in,out] engine  Engine in INIT state.
 * @retval STATUS_OK              Transition to STARTING succeeded.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or not in INIT state.
 * @retval STATUS_INTERNAL_ERROR   Transition logic failure.
 */
StatusCode engine_start(EngineState *engine);

/**
 * @brief Advance engine by one deterministic simulation tick.
 *
 * Handles STARTING→RUNNING transition, SHUTDOWN state clearing,
 * and autonomous RPM/temperature/oil ramp when running.
 *
 * @requirement REQ-ENG-000
 * @pre engine != NULL
 * @post engine performs one deterministic update step and legal transitions
 * @deterministic yes
 *
 * @param[in,out] engine  Engine state to update.
 * @retval STATUS_OK              Update completed.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 * @retval STATUS_INTERNAL_ERROR   Illegal transition detected.
 */
StatusCode engine_update(EngineState *engine);

/**
 * @brief Attempt a state machine transition.
 *
 * @requirement REQ-ENG-000
 * @pre engine != NULL and requested transition is legal
 * @post engine->mode == target_mode when STATUS_OK is returned
 * @deterministic yes
 *
 * @param[in,out] engine       Engine state to transition.
 * @param[in]     target_mode  Desired target state.
 * @retval STATUS_OK              Transition succeeded.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or illegal transition.
 */
StatusCode engine_transition_mode(EngineState *engine, EngineStateMode target_mode);

/**
 * @brief Get human-readable string for current engine mode.
 *
 * @requirement REQ-ENG-000
 * @pre engine != NULL and mode_string != NULL
 * @post *mode_string points to stable mode text for the current engine mode
 * @deterministic yes
 *
 * @param[in]  engine       Engine state to query.
 * @param[out] mode_string  Receives pointer to static mode name string.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or invalid mode.
 */
StatusCode engine_get_mode_string(const EngineState *engine, const char **mode_string);

/**
 * @brief Retrieve factory-default engine physics configuration.
 * @param[out] config_out  Receives default physics configuration.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode engine_get_default_physics(EnginePhysicsConfig *config_out);

/**
 * @brief Retrieve the currently active engine physics configuration.
 * @param[out] config_out  Receives active physics configuration snapshot.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode engine_get_active_physics(EnginePhysicsConfig *config_out);

/**
 * @brief Apply an engine physics configuration.
 *
 * All fields must be positive and finite. May only be called once
 * before engine_reset_physics().
 *
 * @requirement REQ-ENG-PHY-001
 * @param[in] config  Validated physics configuration to apply.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT Invalid config or already configured.
 */
StatusCode engine_configure_physics(const EnginePhysicsConfig *config);

/**
 * @brief Reset engine physics to factory defaults.
 * @retval STATUS_OK Always succeeds.
 */
StatusCode engine_reset_physics(void);

#endif
