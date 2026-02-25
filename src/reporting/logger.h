#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

#include "status.h"

typedef enum
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

StatusCode log_event(const char *level, const char *message);
StatusCode log_event_with_options(const char *level, const char *message, int32_t use_color);
StatusCode logger_set_level_from_string(const char *level_name);
StatusCode logger_set_level(LogLevel level);
LogLevel logger_get_level(void);
StatusCode logger_log_tick(const char *module, LogLevel level, uint32_t tick, const char *message, int32_t use_color);

#endif
