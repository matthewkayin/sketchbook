#include "logger.h"

#include "core/asserts.h"
#include <SDL3/SDL.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

static FILE* logfile = nullptr;

bool logger_init() {
    // Create log folder path
    const char* log_folder_path = "./logs";
    if (!SDL_CreateDirectory(log_folder_path)) {
        printf("Failed to create log folder path %s.\n", SDL_GetError());
        return false;
    }

    // Open logfile
    char logfile_path[256];
    time_t _time = time(NULL);
    tm _tm = *localtime(&_time);
    sprintf(logfile_path,
        "%s/%d-%02d-%02dT%02d%02d%02d.log",
        log_folder_path,
        _tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday, _tm.tm_hour,
        _tm.tm_min, _tm.tm_sec);

    logfile = fopen(logfile_path, "w");
    if (!logfile) {
        return false;
    }

    return true;
}

void logger_quit() {
    fclose(logfile);
}

void logger_output(LogLevel log_level, const char* message, ...) {
    const char* LOG_PREFIX[4] = {"ERROR", "WARN", "INFO", "DEBUG"};
    const size_t MESSAGE_BUFFER_LENGTH = 32000;
    char out_message[MESSAGE_BUFFER_LENGTH];

    memset(out_message, 0, sizeof(out_message));

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(out_message, MESSAGE_BUFFER_LENGTH, message, arg_ptr);
    va_end(arg_ptr);

    printf("[%s]: %s\n", LOG_PREFIX[log_level], out_message);

    if (logfile) {
        fprintf(logfile, "[%s]: %s\n", LOG_PREFIX[log_level], out_message);
    }
}

void logger_report_assertion_failure(
    const char* expression,
    const char* message,
    const char* file,
    int line
) {
    logger_output(
        LOG_LEVEL_ERROR,
        "Assertion failure: %s, message: '%s', in file %s line %d",
        expression, message, file, line);
}
