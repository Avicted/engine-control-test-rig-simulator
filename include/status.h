#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

typedef enum
{
    STATUS_OK = 0,
    STATUS_INVALID_ARGUMENT,
    STATUS_PARSE_ERROR,
    STATUS_IO_ERROR,
    STATUS_TIMEOUT,
    STATUS_BUFFER_OVERFLOW,
    STATUS_INTERNAL_ERROR
} StatusCode;

typedef enum
{
    SEVERITY_INFO = 0,
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    SEVERITY_FATAL
} Severity;

typedef enum
{
    RECOVERABILITY_RECOVERABLE = 0,
    RECOVERABILITY_NON_RECOVERABLE
} Recoverability;

typedef struct
{
    StatusCode code;
    const char *module;
    const char *function;
    uint32_t tick;
    Severity severity;
    Recoverability recoverability;
} ErrorInfo;

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
