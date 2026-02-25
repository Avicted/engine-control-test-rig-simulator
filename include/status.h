/**
 * @file status.h
 * @brief Unified status codes, severity levels, and error diagnostics.
 *
 * All public API functions return StatusCode. Severity and Recoverability
 * classify errors for upstream fault management.
 */

#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

/**
 * @brief Return codes used by all public API functions.
 */
typedef enum
{
    STATUS_OK = 0,           /**< Operation succeeded. */
    STATUS_INVALID_ARGUMENT, /**< NULL pointer or out-of-range parameter. */
    STATUS_PARSE_ERROR,      /**< Malformed input data or script. */
    STATUS_IO_ERROR,         /**< I/O subsystem failure. */
    STATUS_TIMEOUT,          /**< Sensor data not received within window. */
    STATUS_BUFFER_OVERFLOW,  /**< Bounded queue is full. */
    STATUS_INTERNAL_ERROR    /**< Unexpected internal logic error. */
} StatusCode;

/**
 * @brief Severity classification for diagnostic events.
 */
typedef enum
{
    SEVERITY_INFO = 0, /**< Informational. */
    SEVERITY_WARNING,  /**< Condition requires attention. */
    SEVERITY_ERROR,    /**< Recoverable error. */
    SEVERITY_FATAL     /**< Non-recoverable error. */
} Severity;

/**
 * @brief Recoverability classification for fault handling.
 */
typedef enum
{
    RECOVERABILITY_RECOVERABLE = 0, /**< System can recover autonomously. */
    RECOVERABILITY_NON_RECOVERABLE  /**< Manual intervention required. */
} Recoverability;

/**
 * @brief Structured diagnostic record for the most recent error.
 */
typedef struct
{
    StatusCode code;               /**< Status code of the error. */
    const char *module;            /**< Originating module name. */
    const char *function;          /**< Originating function name. */
    uint32_t tick;                 /**< Simulation tick when error occurred. */
    Severity severity;             /**< Severity classification. */
    Recoverability recoverability; /**< Recovery classification. */
} ErrorInfo;

/**
 * @brief Convert a StatusCode to its string representation.
 * @param[in] code  Status code to convert.
 * @return Pointer to static string name (e.g., "STATUS_OK").
 */
static inline const char *status_code_to_string(StatusCode code)
{
    switch (code)
    {
    case STATUS_OK:
        return "STATUS_OK";
    case STATUS_INVALID_ARGUMENT:
        return "STATUS_INVALID_ARGUMENT";
    case STATUS_PARSE_ERROR:
        return "STATUS_PARSE_ERROR";
    case STATUS_IO_ERROR:
        return "STATUS_IO_ERROR";
    case STATUS_TIMEOUT:
        return "STATUS_TIMEOUT";
    case STATUS_BUFFER_OVERFLOW:
        return "STATUS_BUFFER_OVERFLOW";
    case STATUS_INTERNAL_ERROR:
        return "STATUS_INTERNAL_ERROR";
    default:
        return "STATUS_UNKNOWN";
    }
}

static inline const char *severity_to_string(Severity severity)
{
    switch (severity)
    {
    case SEVERITY_INFO:
        return "INFO";
    case SEVERITY_WARNING:
        return "WARNING";
    case SEVERITY_ERROR:
        return "ERROR";
    case SEVERITY_FATAL:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

static inline const char *recoverability_to_string(Recoverability recoverability)
{
    switch (recoverability)
    {
    case RECOVERABILITY_RECOVERABLE:
        return "RECOVERABLE";
    case RECOVERABILITY_NON_RECOVERABLE:
        return "NON_RECOVERABLE";
    default:
        return "UNKNOWN";
    }
}

static inline Recoverability status_code_default_recoverability(StatusCode code)
{
    if ((code == STATUS_INTERNAL_ERROR) || (code == STATUS_IO_ERROR))
    {
        return RECOVERABILITY_NON_RECOVERABLE;
    }

    return RECOVERABILITY_RECOVERABLE;
}

#endif
