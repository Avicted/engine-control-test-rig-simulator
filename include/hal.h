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
#define HAL_SENSOR_FRAME_ID 0x100U
#define HAL_SENSOR_ERROR_FRAME_ID 0x10EU
/** @brief Redundant temperature sensor B frame ID for dual-channel voting. */
#define HAL_SENSOR_TEMP_B_FRAME_ID 0x110U
/** @brief Maximum acceptable disagreement between temperature channels (deg C). */
#define HAL_SENSOR_VOTING_TOLERANCE 5.0f

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

#endif
