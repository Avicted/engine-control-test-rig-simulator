#include <stdio.h>

#include "engine.h"
#include "output.h"

StatusCode output_write_line(const char *line)
{
    int32_t write_result;

    if (line == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    write_result = fputs(line, stdout);
    if (write_result < 0)
    {
        return STATUS_IO_ERROR;
    }

    return STATUS_OK;
}
