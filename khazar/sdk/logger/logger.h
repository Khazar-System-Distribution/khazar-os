#ifndef AI_SDK_LOGGER_H
#define AI_SDK_LOGGER_H

#include "../include/common.h"

void logger_init(const char *filepath, log_level_t level);
void logger_set_level(log_level_t level);
void logger_log(log_level_t level, const char *module, const char *fmt, ...);
void logger_cleanup(void);

#define log_trace(module, ...) logger_log(LOG_TRACE, module, __VA_ARGS__)
#define log_debug(module, ...) logger_log(LOG_DEBUG, module, __VA_ARGS__)
#define log_info(module, ...)  logger_log(LOG_INFO,  module, __VA_ARGS__)
#define log_warn(module, ...)  logger_log(LOG_WARN,  module, __VA_ARGS__)
#define log_error(module, ...) logger_log(LOG_ERROR, module, __VA_ARGS__)
#define log_fatal(module, ...) logger_log(LOG_FATAL, module, __VA_ARGS__)

#endif
