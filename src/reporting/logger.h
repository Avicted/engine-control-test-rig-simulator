/**
 * @file logger.h
 * @brief Level-filtered, tick-prefixed logging subsystem.
 *
 * Provides deterministic, CI-aware logging with optional ANSI color.
 * Debug output is suppressed when the CI environment variable is set.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

#include "status.h"

/**
 * @brief Log verbosity levels, ordered from most to least verbose.
 */
typedef enum
{
    LOG_LEVEL_DEBUG = 0, /**< Verbose debug output (suppressed in CI). */
    LOG_LEVEL_INFO,      /**< Informational messages. */
    LOG_LEVEL_WARN,      /**< Warning conditions. */
    LOG_LEVEL_ERROR      /**< Error conditions. */
} LogLevel;

/**
 * @brief Log an event at the specified level string.
 * @param[in] level    Level name ("DEBUG", "INFO", "WARN", "ERROR").
 * @param[in] message  Message text to log.
 * @retval STATUS_OK              Message logged or suppressed by level.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode log_event(const char *level, const char *message);

/**
 * @brief Log an event with explicit color control.
 * @param[in] level      Level name string.
 * @param[in] message    Message text to log.
 * @param[in] use_color  Non-zero to emit ANSI color codes.
 * @retval STATUS_OK              Message logged or suppressed.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode log_event_with_options(const char *level, const char *message, int32_t use_color);

/**
 * @brief Set log level from a string name.
 * @param[in] level_name  Case-insensitive level name.
 * @retval STATUS_OK              Level set.
 * @retval STATUS_INVALID_ARGUMENT NULL or unrecognized name.
 */
StatusCode logger_set_level_from_string(const char *level_name);

/**
 * @brief Set the active log level.
 * @param[in] level  New minimum log level.
 * @retval STATUS_OK              Level set.
 * @retval STATUS_INVALID_ARGUMENT Level out of range.
 */
StatusCode logger_set_level(LogLevel level);

/**
 * @brief Get the current log level.
 * @return Current LogLevel value.
 */
LogLevel logger_get_level(void);

/**
 * @brief Log a tick-prefixed message with module context.
 * @param[in] module     Module name prefix.
 * @param[in] level      Log level.
 * @param[in] tick       Current simulation tick.
 * @param[in] message    Message text.
 * @param[in] use_color  Non-zero to emit ANSI color codes.
 * @retval STATUS_OK              Message logged or suppressed.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer.
 */
StatusCode logger_log_tick(const char *module, LogLevel level, uint32_t tick, const char *message, int32_t use_color);

#endif
