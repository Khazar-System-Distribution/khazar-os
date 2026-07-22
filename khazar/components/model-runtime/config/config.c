#include "config.h"
#include "logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE "config"

static config_t g_config;

void config_set_defaults(void) {
    memset(&g_config, 0, sizeof(g_config));
    strncpy(g_config.socket_path, SOCKET_PATH_DEFAULT, sizeof(g_config.socket_path) - 1);
    g_config.max_connections = 16;
    strncpy(g_config.model_path, DEFAULT_MODEL_PATH, sizeof(g_config.model_path) - 1);
    g_config.context_size = 2048;
    g_config.default_max_tokens = DEFAULT_MAX_TOKENS;
    g_config.default_temperature = DEFAULT_TEMPERATURE;
    g_config.default_top_p = DEFAULT_TOP_P;
    g_config.num_threads = 4;
    g_config.log_level = LOG_INFO;
    g_config.daemonize = false;
    strncpy(g_config.backend, "mock", sizeof(g_config.backend) - 1);
}

int config_load(const char *path) {
    config_set_defaults();
    if (!path) { log_info(MODULE, "no config, using defaults"); return 0; }

    FILE *f = fopen(path, "r");
    if (!f) { log_warn(MODULE, "cannot open '%s', using defaults", path); return -1; }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        char k[128] = {0}, v[384] = {0};
        if (sscanf(line, "%127[^=]=%383[^\n]", k, v) != 2) continue;

        char *key = k; while (*key == ' ') key++;
        char *val = v; while (*val == ' ') val++;
        size_t vl = strlen(val);
        if (vl >= 2 && val[0] == '"' && val[vl-1] == '"') { val[vl-1] = '\0'; val++; }

        if (strcmp(key, "socket_path") == 0) strncpy(g_config.socket_path, val, sizeof(g_config.socket_path) - 1);
        else if (strcmp(key, "max_connections") == 0) g_config.max_connections = atoi(val);
        else if (strcmp(key, "model_path") == 0) strncpy(g_config.model_path, val, sizeof(g_config.model_path) - 1);
        else if (strcmp(key, "context_size") == 0) g_config.context_size = atoi(val);
        else if (strcmp(key, "default_max_tokens") == 0) g_config.default_max_tokens = atoi(val);
        else if (strcmp(key, "default_temperature") == 0) g_config.default_temperature = (float)atof(val);
        else if (strcmp(key, "default_top_p") == 0) g_config.default_top_p = (float)atof(val);
        else if (strcmp(key, "num_threads") == 0) g_config.num_threads = atoi(val);
        else if (strcmp(key, "log_level") == 0) g_config.log_level = atoi(val);
        else if (strcmp(key, "daemonize") == 0) g_config.daemonize = strcmp(val, "true") == 0;
        else if (strcmp(key, "backend") == 0) strncpy(g_config.backend, val, sizeof(g_config.backend) - 1);
    }
    fclose(f);
    log_info(MODULE, "config loaded from %s", path);
    return 0;
}

config_t *config_get(void) { return &g_config; }

void config_print(config_t *cfg) {
    if (!cfg) return;
    log_info(MODULE, "socket_path: %s", cfg->socket_path);
    log_info(MODULE, "model_path: %s", cfg->model_path);
    log_info(MODULE, "backend: %s", cfg->backend);
    log_info(MODULE, "context_size: %d", cfg->context_size);
    log_info(MODULE, "num_threads: %d", cfg->num_threads);
}
