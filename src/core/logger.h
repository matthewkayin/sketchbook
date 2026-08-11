#pragma once

#include "defines.h"

#if SBK_RELEASE
    #define SBK_LOG_LEVEL LOG_LEVEL_INFO
#else
    #define SBK_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

enum LogLevel {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
};

bool logger_init();
void logger_quit();

void logger_output(LogLevel log_level, const char* message, ...);

#if SBK_LOG_LEVEL >= LOG_LEVEL_ERROR
    #define log_error(message, ...)                                              \
        logger_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#else
    #define log_error(message, ...)
#endif

#if SBK_LOG_LEVEL >= LOG_LEVEL_WARN
    #define log_warn(message, ...)                                              \
        logger_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
    #define log_warn(message, ...)
#endif

#if SBK_LOG_LEVEL >= LOG_LEVEL_INFO
    #define log_info(message, ...)                                              \
        logger_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
    #define log_info(message, ...)
#endif

#if SBK_LOG_LEVEL >= LOG_LEVEL_DEBUG
    #define log_debug(message, ...)                                             \
        logger_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
    #define log_debug(message, ...)
#endif
