#ifndef CONFIG_H
#define CONFIG_H

#include "../include/common.h"

typedef struct {
    char     socket_path[256];
    int      max_connections;
    int      request_timeout_ms;
    bool     enable_metrics;
    log_level_t log_level;
    char     log_file[256];
    char     rule_engine_socket[256];
    char     policy_engine_socket[256];
    char     classifier_socket[256];
} config_t;

int  config_load(const char *path, config_t *cfg);
void config_print(const config_t *cfg);

#endif
