#ifndef CONFIG_H
#define CONFIG_H

#include "../include/common.h"

typedef struct {
    char socket_path[MAX_SOCKET_LEN];
    char model_runtime_socket[MAX_SOCKET_LEN];
    int  max_connections;
    int  log_level;
    bool daemonize;
    bool fallback_to_local;
} config_t;

void        config_set_defaults(void);
int         config_load(const char *path);
config_t   *config_get(void);
void        config_print(config_t *cfg);

#endif
