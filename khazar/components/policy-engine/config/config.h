#ifndef CONFIG_H
#define CONFIG_H

#include "../include/common.h"

typedef struct {
    char socket_path[MAX_SOCKET_LEN];
    int  max_connections;
    char policy_dir[MAX_POLICY_PATH_LEN];
    int  log_level;
    bool daemonize;
} config_t;

void        config_set_defaults(void);
int         config_load(const char *path);
config_t   *config_get(void);
void        config_print(config_t *cfg);

#endif
