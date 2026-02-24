#ifndef LOGGER_H
#define LOGGER_H

#include "status.h"

StatusCode log_event(const char *level, const char *message);
StatusCode log_event_with_options(const char *level, const char *message, int use_color);

#endif
