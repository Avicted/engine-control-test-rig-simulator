#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

#define LOG_BUFFER_SIZE 128
#define LEVEL_BUFFER_SIZE 24

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"

static LogLevel g_min_log_level = LOG_LEVEL_INFO;
static int32_t safe_level_equal(const char *lhs, const char *rhs);

static LogLevel level_from_string(const char *level)
{
    if (safe_level_equal(level, "DEBUG") != 0)
    {
        return LOG_LEVEL_DEBUG;
    }
    if (safe_level_equal(level, "INFO") != 0)
    {
        return LOG_LEVEL_INFO;
    }
    if (safe_level_equal(level, "WARN") != 0)
    {
        return LOG_LEVEL_WARN;
    }
    return LOG_LEVEL_ERROR;
}

static const char *level_to_string(LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        return "DEBUG";
    case LOG_LEVEL_INFO:
        return "INFO";
    case LOG_LEVEL_WARN:
        return "WARN";
    case LOG_LEVEL_ERROR:
        return "ERROR";
    default:
        return "ERROR";
    }
}

static int32_t should_emit_level(LogLevel level)
{
    return (level >= g_min_log_level) ? 1 : 0;
}

static int32_t safe_level_equal(const char *lhs, const char *rhs)
{
    if ((lhs == NULL) || (rhs == NULL))
    {
        return 0;
    }

    return (strcmp(lhs, rhs) == 0) ? 1 : 0;
}

static const char *level_to_display(const char *level, int32_t use_color)
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

StatusCode logger_set_level(LogLevel level)
{
    const char *ci_env;

    if ((level < LOG_LEVEL_DEBUG) || (level > LOG_LEVEL_ERROR))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    ci_env = getenv("CI");
    if ((ci_env != NULL) && (level == LOG_LEVEL_DEBUG))
    {
        g_min_log_level = LOG_LEVEL_INFO;
        return STATUS_OK;
    }

    g_min_log_level = level;
    return STATUS_OK;
}

StatusCode logger_set_level_from_string(const char *level_name)
{
    if (level_name == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if ((safe_level_equal(level_name, "DEBUG") == 0) && (safe_level_equal(level_name, "INFO") == 0) &&
        (safe_level_equal(level_name, "WARN") == 0) && (safe_level_equal(level_name, "ERROR") == 0))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    return logger_set_level(level_from_string(level_name));
}

LogLevel logger_get_level(void)
{
    return g_min_log_level;
}

StatusCode log_event(const char *level, const char *message)
{
    return log_event_with_options(level, message, 0);
}

StatusCode log_event_with_options(const char *level, const char *message, int32_t use_color)
{
    if (should_emit_level(level_from_string(level)) == 0)
    {
        return STATUS_OK;
    }

    char buffer[LOG_BUFFER_SIZE];
    char level_buffer[LEVEL_BUFFER_SIZE];
    const char *display_level;
    int32_t level_written;
    int32_t written;
    int32_t print_result;

    if ((level == NULL) || (message == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    level_written = snprintf(level_buffer, sizeof(level_buffer), "%s", level);
    if ((level_written < 0) || (level_written >= (int32_t)sizeof(level_buffer)))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    display_level = level_to_display(level_buffer, use_color);
    written = snprintf(buffer, sizeof(buffer), "[%s] %s\n", display_level, message);
    if ((written < 0) || (written >= (int32_t)sizeof(buffer)))
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

StatusCode logger_log_tick(const char *module, LogLevel level, uint32_t tick, const char *message, int32_t use_color)
{
    char buffer[LOG_BUFFER_SIZE];
    const char *raw_level;
    const char *display_level;
    int32_t written;
    int32_t print_result;

    if ((module == NULL) || (message == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (should_emit_level(level) == 0)
    {
        return STATUS_OK;
    }

    raw_level = level_to_string(level);
    display_level = level_to_display(raw_level, use_color);

    written = snprintf(buffer,
                       sizeof(buffer),
                       "[TICK %03u][%s][%s] %s\n",
                       tick,
                       module,
                       display_level,
                       message);
    if ((written < 0) || (written >= (int32_t)sizeof(buffer)))
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
