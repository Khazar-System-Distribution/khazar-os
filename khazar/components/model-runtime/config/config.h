#ifndef CONFIG_H
#define CONFIG_H

#include "../include/common.h"

typedef struct {
    char socket_path[MAX_SOCKET_LEN];
    int  max_connections;
    char model_path[MAX_MODEL_PATH_LEN];
    int  context_size;
    int  default_max_tokens;
    float default_temperature;
    float default_top_p;
    int  num_threads;
    int  log_level;
    bool daemonize;
    char backend[32];
} config_t;

void        config_set_defaults(void);
int         config_load(const char *path);
config_t   *config_get(void);
void        config_print(config_t *cfg);

#endif
