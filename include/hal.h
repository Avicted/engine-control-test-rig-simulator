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
_Static_assert(sizeof(((BusFrame *)0)->data) == 8U, "BusFrame data must be 8 bytes");
_Static_assert(sizeof(((BusFrame *)0)->dlc) == 1U, "BusFrame dlc must be 1 byte");
_Static_assert(sizeof(BusFrame) == 13U, "Unexpected frame size");

/*
 * @requirement REQ-ENG-IO-001
 * @pre none
 * @post HAL subsystem is ready for deterministic simulator I/O calls
 * @deterministic yes
 */
StatusCode hal_init(void);

/*
 * @requirement REQ-ENG-IO-001
 * @pre HAL was initialized
 * @post HAL subsystem is cleanly shut down
 * @deterministic yes
 */
StatusCode hal_shutdown(void);

/*
 * @requirement REQ-ENG-IO-002
 * @pre frame != NULL
 * @post sensor frame or supported sensor error frame is queued in bounded deterministic RX queue when STATUS_OK is returned
 * @deterministic yes
 */
StatusCode hal_ingest_sensor_frame(const HAL_Frame *frame, uint32_t tick);

/*
 * @requirement REQ-ENG-IO-003
 * @pre tick monotonically increases, frame_out != NULL
 * @post returns STATUS_OK and decodes one queued sensor frame, STATUS_PARSE_ERROR for queued sensor error frames, or STATUS_TIMEOUT on deterministic timeout
 * @deterministic yes
 */
StatusCode hal_read_sensors(uint32_t tick, HAL_SensorFrame *frame_out);

/*
 * @requirement REQ-ENG-IO-004
 * @pre sensor_frame != NULL, frame_out != NULL
 * @post frame_out contains deterministic transport encoding of sensor_frame
 * @deterministic yes
 */
StatusCode hal_encode_sensor_frame(const HAL_SensorFrame *sensor_frame, HAL_Frame *frame_out);

/*
 * @requirement REQ-ENG-001
 * @pre frame != NULL, engine != NULL, frame is validated
 * @post engine state fields are updated from frame when STATUS_OK is returned
 * @deterministic yes
 */
StatusCode hal_apply_sensors(const HAL_SensorFrame *frame, EngineState *engine);

/*
 * @requirement REQ-ENG-IO-005
 * @pre frame != NULL
 * @post returns STATUS_OK when frame metadata is transport-safe for simulator bus ingestion
 * @deterministic yes
 */
StatusCode hal_receive_bus(const HAL_Frame *frame);

/*
 * @requirement REQ-ENG-IO-006
 * @pre frame != NULL
 * @post returns STATUS_OK when frame metadata is transport-safe for simulator bus egress
 * @deterministic yes
 */
StatusCode hal_transmit_bus(const HAL_Frame *frame);

/*
 * @requirement REQ-ENG-003
 * @pre frame != NULL
 * @post actuator output is emitted when requested and function returns STATUS_OK
 * @deterministic yes
 */
StatusCode hal_write_actuators(const HAL_ControlFrame *frame);

/*
 * @requirement REQ-ENG-DIAG-001
 * @pre error_info != NULL
 * @post error_info contains latest HAL diagnostic metadata
 * @deterministic yes
 */
StatusCode hal_get_last_error(ErrorInfo *error_info);

#endif
