#include <stdio.h>

#include "hal.h"

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
    if (frame == (const HAL_SensorFrame *)0)
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
