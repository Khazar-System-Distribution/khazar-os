#include "logger/logger.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    logger_init(LOG_TRACE);
    logger_set_level(LOG_DEBUG);

    log_info("test", "info message");
    log_debug("test", "debug message: %d", 42);
    log_warn("test", "warning");
    log_error("test", "error occurred");

    fprintf(stderr, "logger: ALL TESTS PASSED\n");
    logger_cleanup();
    return 0;
}
