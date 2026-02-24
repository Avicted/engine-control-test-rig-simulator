#ifndef LOGGER_H
#define LOGGER_H

int log_event(const char *level, const char *message);
int log_event_with_options(const char *level, const char *message, int use_color);

#endif
