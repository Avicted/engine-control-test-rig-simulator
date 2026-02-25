#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#include "engine.h"
#include "status.h"

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

typedef struct
{
    uint32_t message_id;
    uint8_t dlc;
    uint8_t payload[8];
} HAL_BusFrame;

_Static_assert(sizeof(uint8_t) == 1U, "uint8_t must be 8-bit");
_Static_assert(sizeof(((HAL_BusFrame *)0)->payload) == 8U, "HAL_BusFrame payload must be 8 bytes");
_Static_assert(sizeof(((HAL_BusFrame *)0)->dlc) == 1U, "HAL_BusFrame dlc must be 1 byte");

/*
 * PRE: none.
 * POST: HAL subsystem is ready for deterministic simulator I/O calls.
 */
StatusCode hal_init(void);

/*
 * PRE: HAL was initialized.
 * POST: HAL subsystem is cleanly shut down.
 */
StatusCode hal_shutdown(void);

/*
 * PRE: frame != NULL.
 * POST: returns STATUS_OK only when frame fields are finite and within configured bounds.
 */
StatusCode hal_read_sensors(const HAL_SensorFrame *frame);

/*
 * PRE: frame != NULL, engine != NULL, frame is valid for hal_read_sensors().
 * POST: engine state fields are updated from frame when STATUS_OK is returned.
 */
StatusCode hal_apply_sensors(const HAL_SensorFrame *frame, EngineState *engine);

/*
 * PRE: frame != NULL.
 * POST: returns STATUS_OK when frame metadata is transport-safe for simulator bus ingestion.
 */
StatusCode hal_receive_bus(const HAL_BusFrame *frame);

/*
 * PRE: frame != NULL.
 * POST: returns STATUS_OK when frame metadata is transport-safe for simulator bus egress.
 */
StatusCode hal_transmit_bus(const HAL_BusFrame *frame);

/*
 * PRE: frame != NULL.
 * POST: actuator output is emitted when requested and function returns STATUS_OK.
 */
StatusCode hal_write_actuators(const HAL_ControlFrame *frame);

#endif
