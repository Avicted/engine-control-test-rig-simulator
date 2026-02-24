#include <stdio.h>
#include <string.h>

#include "logger.h"

#define LOG_BUFFER_SIZE 128
#define LEVEL_BUFFER_SIZE 24

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"

static int safe_level_equal(const char *lhs, const char *rhs)
{
    if ((lhs == (const char *)0) || (rhs == (const char *)0))
    {
        return 0;
    }

    return (strcmp(lhs, rhs) == 0) ? 1 : 0;
}

static const char *level_to_display(const char *level, int use_color)
{
    if (use_color == 0)
    {
        return level;
    }

    if (safe_level_equal(level, "INFO") != 0)
    {
        return ANSI_GREEN "INFO" ANSI_RESET;
    }
    if (safe_level_equal(level, "WARN") != 0)
    {
        return ANSI_YELLOW "WARN" ANSI_RESET;
    }
    if (safe_level_equal(level, "ERROR") != 0)
    {
        return ANSI_RED "ERROR" ANSI_RESET;
    }

    return level;
}

StatusCode log_event(const char *level, const char *message)
{
    return log_event_with_options(level, message, 0);
}

StatusCode log_event_with_options(const char *level, const char *message, int use_color)
{
    char buffer[LOG_BUFFER_SIZE];
    char level_buffer[LEVEL_BUFFER_SIZE];
    const char *display_level;
    int level_written;
    int written;
    int print_result;

    if ((level == (const char *)0) || (message == (const char *)0))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    level_written = snprintf(level_buffer, sizeof(level_buffer), "%s", level);
    if ((level_written < 0) || (level_written >= (int)sizeof(level_buffer)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    display_level = level_to_display(level_buffer, use_color);
    written = snprintf(buffer, sizeof(buffer), "[%s] %s\n", display_level, message);
    if ((written < 0) || (written >= (int)sizeof(buffer)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    print_result = fputs(buffer, stdout);
    if (print_result < 0)
    {
        return STATUS_IO_ERROR;
    }

    return STATUS_OK;
}
