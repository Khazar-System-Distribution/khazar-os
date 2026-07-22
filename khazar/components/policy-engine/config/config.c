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
    g_config.max_connections = 64;
    strncpy(g_config.policy_dir, POLICY_DEFAULT_DIR, sizeof(g_config.policy_dir) - 1);
    g_config.log_level = LOG_INFO;
    g_config.daemonize = false;
}

int config_load(const char *path) {
    config_set_defaults();

    if (!path) {
        log_info(MODULE, "no config path, using defaults");
        return 0;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        log_warn(MODULE, "cannot open config '%s', using defaults", path);
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        char key[128] = {0}, value[384] = {0};
        if (sscanf(line, "%127[^=]=%383[^\n]", key, value) != 2) continue;

        char *k = key;
        while (*k == ' ') k++;
        char *ke = k + strlen(k) - 1;
        while (ke > k && *ke == ' ') { *ke = '\0'; ke--; }

        char *v = value;
        while (*v == ' ') v++;
        size_t vl = strlen(v);
        if (vl >= 2 && v[0] == '"' && v[vl - 1] == '"') {
            v[vl - 1] = '\0';
            v++;
        }

        if (strcmp(k, "socket_path") == 0)
            strncpy(g_config.socket_path, v, sizeof(g_config.socket_path) - 1);
        else if (strcmp(k, "max_connections") == 0)
            g_config.max_connections = atoi(v);
        else if (strcmp(k, "policy_dir") == 0)
            strncpy(g_config.policy_dir, v, sizeof(g_config.policy_dir) - 1);
        else if (strcmp(k, "log_level") == 0)
            g_config.log_level = atoi(v);
        else if (strcmp(k, "daemonize") == 0)
            g_config.daemonize = (strcmp(v, "true") == 0);
    }

    fclose(f);
    log_info(MODULE, "config loaded from %s", path);
    return 0;
}

config_t *config_get(void) {
    return &g_config;
}

void config_print(config_t *cfg) {
    if (!cfg) return;
    log_info(MODULE, "socket_path: %s", cfg->socket_path);
    log_info(MODULE, "max_connections: %d", cfg->max_connections);
    log_info(MODULE, "policy_dir: %s", cfg->policy_dir);
    log_info(MODULE, "log_level: %d", cfg->log_level);
    log_info(MODULE, "daemonize: %s", cfg->daemonize ? "true" : "false");
}
