#include <stdio.h>

#include "logger.h"

#define LOG_BUFFER_SIZE 128

int log_event(const char *level, const char *message)
{
    char buffer[LOG_BUFFER_SIZE];
    int written;
    int print_result;

    if ((level == (const char *)0) || (message == (const char *)0))
    {
        return -1;
    }

    written = snprintf(buffer, sizeof(buffer), "[%s] %s\n", level, message);
    if ((written < 0) || (written >= (int)sizeof(buffer)))
    {
        return -1;
    }

    print_result = fputs(buffer, stdout);
    if (print_result < 0)
    {
        return -1;
    }

    return 0;
}
