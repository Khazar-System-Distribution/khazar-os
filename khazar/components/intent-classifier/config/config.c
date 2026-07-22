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
    strncpy(g_config.model_runtime_socket, RUNTIME_SOCKET_DEFAULT, sizeof(g_config.model_runtime_socket) - 1);
    g_config.max_connections = 16;
    g_config.log_level = LOG_INFO;
    g_config.daemonize = false;
    g_config.fallback_to_local = true;
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
        else if (strcmp(key, "model_runtime_socket") == 0) strncpy(g_config.model_runtime_socket, val, sizeof(g_config.model_runtime_socket) - 1);
        else if (strcmp(key, "max_connections") == 0) g_config.max_connections = atoi(val);
        else if (strcmp(key, "log_level") == 0) g_config.log_level = atoi(val);
        else if (strcmp(key, "daemonize") == 0) g_config.daemonize = strcmp(val, "true") == 0;
        else if (strcmp(key, "fallback_to_local") == 0) g_config.fallback_to_local = strcmp(val, "true") == 0;
    }
    fclose(f);
    log_info(MODULE, "config loaded from %s", path);
    return 0;
}

config_t *config_get(void) { return &g_config; }

void config_print(config_t *cfg) {
    if (!cfg) return;
    log_info(MODULE, "socket_path: %s", cfg->socket_path);
    log_info(MODULE, "model_runtime_socket: %s", cfg->model_runtime_socket);
    log_info(MODULE, "max_connections: %d", cfg->max_connections);
    log_info(MODULE, "log_level: %d", cfg->log_level);
    log_info(MODULE, "daemonize: %s", cfg->daemonize ? "true" : "false");
    log_info(MODULE, "fallback_to_local: %s", cfg->fallback_to_local ? "true" : "false");
}
