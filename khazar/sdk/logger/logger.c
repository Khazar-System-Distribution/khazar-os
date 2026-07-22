#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static log_level_t current_level = LOG_INFO;
static FILE *log_file = NULL;

static const char *level_str[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

void logger_init(const char *filepath, log_level_t level) {
    current_level = level;
    pthread_mutex_init(&log_mutex, NULL);

    if (filepath && filepath[0] != '\0') {
        log_file = fopen(filepath, "a");
        if (!log_file) {
            fprintf(stderr, "logger: cannot open '%s', using stderr\n", filepath);
            log_file = stderr;
        }
    }
}

void logger_set_level(log_level_t level) {
    current_level = level;
}

static void get_timestamp(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", tm);
}

void logger_log(log_level_t level, const char *module, const char *fmt, ...) {
    if (level < current_level)
        return;

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    char msg[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    pthread_mutex_lock(&log_mutex);

    if (log_file) {
        fprintf(log_file, "%s %s %s %s\n", timestamp, level_str[level],
                module ? module : "", msg);
        fflush(log_file);
    } else {
        fprintf(stderr, "%s %s %s %s\n", timestamp, level_str[level],
                module ? module : "", msg);
    }

    pthread_mutex_unlock(&log_mutex);

    if (level == LOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}

void logger_cleanup(void) {
    pthread_mutex_lock(&log_mutex);
    if (log_file && log_file != stderr) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
    pthread_mutex_destroy(&log_mutex);
}
