/**
 * @file hal.h
 * @brief Hardware Abstraction Layer - CAN-like frame encoding/decoding, sensor I/O, bus queues.
 */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#include "engine.h"
#include "status.h"

#if defined(__clang__) || defined(__GNUC__)
#define HAL_PACKED __attribute__((packed))
#else
#define HAL_PACKED
#endif

#define HAL_MAX_RX_FRAMES 32U
#define HAL_MAX_TX_FRAMES 32U
#define HAL_SENSOR_TIMEOUT_TICKS 3U

/**
 * @brief Centralized frame ID registry.
 *
 * All valid frame IDs are declared here. Unknown IDs are rejected
 * deterministically by HAL ingestion and validation functions.
 * New frame types must be added to this enum and to
 * hal_expected_dlc_for_id().
 */
typedef enum
{
    FRAME_ID_SENSOR = 0x100U,       /**< Primary sensor data frame.         DLC = 8. */
    FRAME_ID_SENSOR_ERROR = 0x10EU, /**< Sensor error / diagnostic frame.   DLC = 1. */
    FRAME_ID_SENSOR_TEMP_B = 0x110U /**< Redundant temperature channel B.   DLC = 2. */
} FrameId;

/** @brief Number of entries in the FrameId registry. */
#define FRAME_ID_REGISTRY_COUNT 3U

/* Backward-compatible ID macros (prefer FrameId enum in new code). */
#define HAL_SENSOR_FRAME_ID ((uint32_t)FRAME_ID_SENSOR)
#define HAL_SENSOR_ERROR_FRAME_ID ((uint32_t)FRAME_ID_SENSOR_ERROR)
/** @brief Redundant temperature sensor B frame ID for dual-channel voting. */
#define HAL_SENSOR_TEMP_B_FRAME_ID ((uint32_t)FRAME_ID_SENSOR_TEMP_B)
/** @brief Maximum acceptable disagreement between temperature channels (deg C). */
#define HAL_SENSOR_VOTING_TOLERANCE 5.0f

/** @brief Frame staleness threshold in ticks.
 *  A frame ID with no reception for this many ticks is considered stale. */
#define HAL_FRAME_STALE_THRESHOLD_TICKS 10U

/**
 * @name Queue Overflow Policy - DROP NEWEST
 *
 * When any internal queue (sensor RX, bus RX, bus TX) is full, the
 * new frame is **rejected** with STATUS_BUFFER_OVERFLOW and a
 * structured error diagnostic is recorded via hal_set_error().
 * Existing queue contents are preserved intact.
 *
 * Rationale: deterministic behavior - the oldest data already in the
 * queue is never silently discarded, and callers receive an explicit
 * error to handle.
 */
/** @{ */
/** @} */

/** @brief Default watchdog timeout in ticks. 0 = disabled. */
#define HAL_WATCHDOG_DEFAULT_TICKS 0U
/** @brief Maximum allowed watchdog timeout in ticks. */
#define HAL_WATCHDOG_MAX_TICKS 100U

typedef struct
{
    float rpm;
    float temperature;
    float oil_pressure;
    int32_t is_running;
} HAL_SensorFrame;

typedef struct
{
    float control_output;
    int32_t emit_control_line;
} HAL_ControlFrame;

typedef struct HAL_PACKED
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} BusFrame;

typedef BusFrame HAL_Frame;
typedef BusFrame HAL_BusFrame;

_Static_assert(sizeof(uint8_t) == 1U, "uint8_t must be 8-bit");
_Static_assert(sizeof(((BusFrame){0}).data) == 8U, "BusFrame data must be 8 bytes");
_Static_assert(sizeof(((BusFrame){0}).dlc) == 1U, "BusFrame dlc must be 1 byte");
_Static_assert(sizeof(BusFrame) == 13U, "Unexpected frame size");
_Static_assert(sizeof(HAL_SensorFrame) == 16U, "HAL_SensorFrame ABI contract changed");
_Static_assert(sizeof(HAL_ControlFrame) == 8U, "HAL_ControlFrame ABI contract changed");

/**
 * @brief Initialize HAL subsystem - reset all queues and state.
 *
 * @requirement REQ-ENG-IO-001
 * @pre none
 * @post HAL subsystem is ready for deterministic simulator I/O calls
 * @deterministic yes
 *
 * @retval STATUS_OK Always succeeds.
 */
StatusCode hal_init(void);

/**
 * @brief Shut down HAL subsystem - reset all queues and state.
 *
 * @requirement REQ-ENG-IO-001
 * @pre HAL was initialized
 * @post HAL subsystem is cleanly shut down
 * @deterministic yes
 *
 * @retval STATUS_OK Always succeeds.
 */
StatusCode hal_shutdown(void);

/**
 * @brief Queue a sensor frame for later decoding by hal_read_sensors().
 *
 * @requirement REQ-ENG-IO-002
 * @pre frame != NULL
 * @post sensor frame or supported sensor error frame is queued in bounded
 *       deterministic RX queue when STATUS_OK is returned
 * @deterministic yes
 *
 * @param[in] frame  CAN-like frame to ingest.
 * @param[in] tick   Current simulation tick.
 * @retval STATUS_OK              Frame queued.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or unsupported frame type.
 * @retval STATUS_BUFFER_OVERFLOW  RX queue full.
 */
StatusCode hal_ingest_sensor_frame(const HAL_Frame *frame, uint32_t tick);

/**
 * @brief Dequeue and decode one sensor frame.
 *
 * @requirement REQ-ENG-IO-003
 * @pre tick monotonically increases, frame_out != NULL
 * @post returns STATUS_OK and decodes one queued sensor frame,
 *       STATUS_PARSE_ERROR for queued sensor error frames,
 *       or STATUS_TIMEOUT on deterministic timeout
 * @deterministic yes
 *
 * @param[in]  tick       Current simulation tick.
 * @param[out] frame_out  Receives decoded sensor data.
 * @retval STATUS_OK              Frame decoded successfully.
 * @retval STATUS_PARSE_ERROR     Corrupt frame or error frame.
 * @retval STATUS_TIMEOUT         No sensor data within timeout window.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or empty queue (non-timeout).
 */
StatusCode hal_read_sensors(uint32_t tick, HAL_SensorFrame *frame_out);

/**
 * @brief Encode sensor data into a CAN-like transport frame.
 *
 * @requirement REQ-ENG-IO-004
 * @pre sensor_frame != NULL, frame_out != NULL
 * @post frame_out contains deterministic transport encoding of sensor_frame
 * @deterministic yes
 *
 * @param[in]  sensor_frame  Validated sensor data to encode.
 * @param[out] frame_out     Receives encoded frame with checksum.
 * @retval STATUS_OK              Encoding succeeded.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or invalid sensor values.
 */
StatusCode hal_encode_sensor_frame(const HAL_SensorFrame *sensor_frame, HAL_Frame *frame_out);

/**
 * @brief Apply decoded sensor values to engine state.
 *
 * @requirement REQ-ENG-001
 * @pre frame != NULL, engine != NULL, frame is validated
 * @post engine state fields are updated from frame when STATUS_OK is returned
 * @deterministic yes
 *
 * @param[in]     frame   Validated sensor frame to apply.
 * @param[in,out] engine  Engine state to update.
 * @retval STATUS_OK              Engine state updated.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or invalid sensor values.
 */
StatusCode hal_apply_sensors(const HAL_SensorFrame *frame, EngineState *engine);

/**
 * @brief Queue a frame on the bus RX queue.
 *
 * @requirement REQ-ENG-IO-005
 * @pre frame != NULL
 * @post returns STATUS_OK when frame metadata is transport-safe for bus ingestion
 * @deterministic yes
 *
 * @param[in] frame  Frame to receive.
 * @retval STATUS_OK              Frame queued.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or DLC > 8.
 * @retval STATUS_BUFFER_OVERFLOW  Queue full.
 */
StatusCode hal_receive_bus(const HAL_Frame *frame);

/**
 * @brief Queue a frame on the bus TX queue.
 *
 * @requirement REQ-ENG-IO-006
 * @pre frame != NULL
 * @post returns STATUS_OK when frame metadata is transport-safe for bus egress
 * @deterministic yes
 *
 * @param[in] frame  Frame to transmit.
 * @retval STATUS_OK              Frame queued.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or DLC > 8.
 * @retval STATUS_BUFFER_OVERFLOW  Queue full.
 */
StatusCode hal_transmit_bus(const HAL_Frame *frame);

/**
 * @brief Emit control output to actuators.
 *
 * @requirement REQ-ENG-003
 * @pre frame != NULL
 * @post actuator output is emitted when requested and function returns STATUS_OK
 * @deterministic yes
 *
 * @param[in] frame  Control output frame.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 * @retval STATUS_IO_ERROR         Output write failure.
 */
StatusCode hal_write_actuators(const HAL_ControlFrame *frame);

/**
 * @brief Retrieve the most recent HAL error diagnostic.
 *
 * @requirement REQ-ENG-DIAG-001
 * @pre error_info != NULL
 * @post error_info contains latest HAL diagnostic metadata
 * @deterministic yes
 *
 * @param[out] error_info  Receives latest error information.
 * @retval STATUS_OK              Success.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode hal_get_last_error(ErrorInfo *error_info);

/**
 * @brief Configure the software watchdog timer.
 *
 * When enabled (timeout_ticks > 0), hal_watchdog_kick() must be called
 * at least every timeout_ticks simulation ticks. If the watchdog expires,
 * hal_watchdog_check() returns STATUS_TIMEOUT and the caller should
 * force engine SHUTDOWN.
 *
 * @requirement REQ-ENG-WD-001
 * @param[in] timeout_ticks  Watchdog period in ticks (0 = disabled).
 * @retval STATUS_OK              Watchdog configured.
 * @retval STATUS_INVALID_ARGUMENT timeout_ticks exceeds HAL_WATCHDOG_MAX_TICKS.
 */
StatusCode hal_watchdog_configure(uint32_t timeout_ticks);

/**
 * @brief Reset (kick) the watchdog timer - called after successful sensor read.
 * @retval STATUS_OK Always succeeds.
 */
StatusCode hal_watchdog_kick(void);

/**
 * @brief Check whether the watchdog has expired.
 *
 * Advances the watchdog counter by one tick. If the counter exceeds the
 * configured timeout, returns STATUS_TIMEOUT (watchdog expired).
 *
 * @requirement REQ-ENG-WD-001
 * @param[in] tick  Current simulation tick (for diagnostics).
 * @retval STATUS_OK              Watchdog not expired (or disabled).
 * @retval STATUS_TIMEOUT         Watchdog expired - force SHUTDOWN.
 */
StatusCode hal_watchdog_check(uint32_t tick);

/**
 * @brief Submit a redundant temperature reading from sensor channel B.
 *
 * Used for dual-channel temperature voting. Must be called after
 * hal_ingest_sensor_frame() for the same tick. The value is stored
 * internally and compared during hal_vote_sensors().
 *
 * @requirement REQ-ENG-VOTE-001
 * @param[in] temperature_b  Temperature reading from channel B (deg C).
 * @param[in] tick           Current simulation tick.
 * @retval STATUS_OK  Reading stored.
 */
StatusCode hal_submit_redundant_temp(float temperature_b, uint32_t tick);

/**
 * @brief Compare primary and redundant temperature channels.
 *
 * Returns STATUS_OK if the channels agree within HAL_SENSOR_VOTING_TOLERANCE,
 * or STATUS_PARSE_ERROR if they disagree (sensor voting failure).
 * If no redundant reading has been submitted, returns STATUS_OK (single-channel mode).
 *
 * @requirement REQ-ENG-VOTE-001
 * @param[in]  primary_temp  Temperature from the primary sensor channel.
 * @param[out] voted_temp    Receives the average of both channels on success,
 *                           or primary_temp if no redundant reading available.
 * @retval STATUS_OK              Channels agree (or single-channel mode).
 * @retval STATUS_PARSE_ERROR     Channels disagree beyond tolerance.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode hal_vote_sensors(float primary_temp, float *voted_temp);

/* ------------------------------------------------------------------ */
/*  Frame ID Registry helpers (Section 1.1 / 1.2)                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Return whether a frame ID belongs to the registry.
 * @param[in] id  Frame ID to check.
 * @return 1 if registered, 0 otherwise.
 */
int32_t hal_frame_id_is_known(uint32_t id);

/**
 * @brief Return the expected DLC for a registered frame ID.
 *
 * @requirement REQ-ENG-IO-007
 * @param[in]  id       Frame ID to query.
 * @param[out] dlc_out  Receives expected DLC.
 * @retval STATUS_OK              Known ID, DLC written.
 * @retval STATUS_INVALID_ARGUMENT Unknown ID or NULL pointer.
 */
StatusCode hal_expected_dlc_for_id(uint32_t id, uint8_t *dlc_out);

/* ------------------------------------------------------------------ */
/*  Frame Aging Model (Section 1.3)                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Record receipt of a frame ID at the given tick.
 *
 * @requirement REQ-ENG-IO-008
 * @param[in] id    Frame ID that was received.
 * @param[in] tick  Current simulation tick.
 * @retval STATUS_OK              Recorded.
 * @retval STATUS_INVALID_ARGUMENT Unknown frame ID.
 */
StatusCode hal_frame_age_record(uint32_t id, uint32_t tick);

/**
 * @brief Query whether a frame ID is stale at the given tick.
 *
 * A frame is stale if it has been received at least once and
 * (current_tick − last_received_tick) > HAL_FRAME_STALE_THRESHOLD_TICKS.
 * A frame that has never been received is not considered stale (it is
 * "unknown", not "aged").
 *
 * @requirement REQ-ENG-IO-008
 * @param[in]  id        Frame ID to check.
 * @param[in]  tick      Current simulation tick.
 * @param[out] stale_out Receives 1 if stale, 0 if fresh or never received.
 * @retval STATUS_OK              Query succeeded.
 * @retval STATUS_INVALID_ARGUMENT Unknown ID or NULL pointer.
 */
StatusCode hal_frame_is_stale(uint32_t id, uint32_t tick, int32_t *stale_out);

#endif
