#include <math.h>
#include <stdio.h>

#include "hal.h"

#define HAL_MIN_RPM 0.0f
#define HAL_MAX_RPM 10000.0f
#define HAL_MIN_TEMP -50.0f
#define HAL_MAX_TEMP 200.0f
#define HAL_MIN_OIL 0.0f
#define HAL_MAX_OIL 10.0f

static StatusCode validate_sensor_frame(const HAL_SensorFrame *frame)
{
    if (frame == (const HAL_SensorFrame *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (!isfinite(frame->rpm) || !isfinite(frame->temperature) || !isfinite(frame->oil_pressure))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if ((frame->rpm < HAL_MIN_RPM) || (frame->rpm > HAL_MAX_RPM) || (frame->temperature < HAL_MIN_TEMP) ||
        (frame->temperature > HAL_MAX_TEMP) || (frame->oil_pressure < HAL_MIN_OIL) ||
        (frame->oil_pressure > HAL_MAX_OIL) || ((frame->is_running != 0) && (frame->is_running != 1)))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

StatusCode hal_init(void)
{
    return STATUS_OK;
}

StatusCode hal_shutdown(void)
{
    return STATUS_OK;
}

StatusCode hal_read_sensors(const HAL_SensorFrame *frame)
{
    return validate_sensor_frame(frame);
}

StatusCode hal_apply_sensors(const HAL_SensorFrame *frame, EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (validate_sensor_frame(frame) != STATUS_OK)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    engine->is_running = frame->is_running;
    engine->rpm = frame->rpm;
    engine->temperature = frame->temperature;
    engine->oil_pressure = frame->oil_pressure;

    return STATUS_OK;
}

StatusCode hal_receive_bus(const HAL_BusFrame *frame)
{
    if (frame == (const HAL_BusFrame *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (frame->dlc > 8U)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

StatusCode hal_transmit_bus(const HAL_BusFrame *frame)
{
    if (frame == (const HAL_BusFrame *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (frame->dlc > 8U)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

StatusCode hal_write_actuators(const HAL_ControlFrame *frame)
{
    if (frame == (const HAL_ControlFrame *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (frame->emit_control_line != 0)
    {
        int written;

        written = printf("CTRL | output=%6.2f%%\n", frame->control_output);
        if (written < 0)
        {
            return STATUS_IO_ERROR;
        }
    }

    return STATUS_OK;
}
