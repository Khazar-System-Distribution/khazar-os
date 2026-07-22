#include "config.h"
#include "logger/logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MODULE "config"

int config_load(const char *path, config_t *cfg) {
    memset(cfg, 0, sizeof(config_t));

    strncpy(cfg->socket_path, DEFAULT_ORCHESTRATOR_SOCKET, sizeof(cfg->socket_path) - 1);
    cfg->max_connections = MAX_CONNECTIONS;
    cfg->request_timeout_ms = REQUEST_TIMEOUT_MS;
    cfg->enable_metrics = true;
    cfg->log_level = LOG_INFO;
    cfg->log_file[0] = '\0';
    strncpy(cfg->rule_engine_socket, "/run/ai-rule-engine.sock", sizeof(cfg->rule_engine_socket) - 1);
    strncpy(cfg->policy_engine_socket, "/run/ai-policy-engine.sock", sizeof(cfg->policy_engine_socket) - 1);
    strncpy(cfg->classifier_socket, "/run/ai-intent-classifier.sock", sizeof(cfg->classifier_socket) - 1);

    if (!path)
        return 0;

    FILE *f = fopen(path, "r");
    if (!f) {
        log_warn(MODULE, "config file not found: %s, using defaults", path);
        return 0;
    }

    char line[512];
    int line_num = 0;
    while (fgets(line, sizeof(line), f)) {
        line_num++;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0')
            continue;

        char *eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = p;
        char *val = eq + 1;

        while (*key == ' ' || *key == '\t') key++;
        char *ke = key + strlen(key) - 1;
        while (ke > key && (*ke == ' ' || *ke == '\t')) *ke-- = '\0';

        while (*val == ' ' || *val == '\t') val++;
        char *ve = val + strlen(val) - 1;
        while (ve > val && (*ve == ' ' || *ve == '\t' || *ve == '\n' || *ve == '\r')) *ve-- = '\0';

        if (strcmp(key, "socket_path") == 0 && val[0] == '"') {
            char *end = strrchr(val + 1, '"');
            if (end) { *end = '\0'; strncpy(cfg->socket_path, val + 1, sizeof(cfg->socket_path) - 1); }
        } else if (strcmp(key, "max_connections") == 0) {
            cfg->max_connections = atoi(val);
        } else if (strcmp(key, "request_timeout_ms") == 0) {
            cfg->request_timeout_ms = atoi(val);
        } else if (strcmp(key, "enable_metrics") == 0) {
            cfg->enable_metrics = strcmp(val, "true") == 0;
        } else if (strcmp(key, "log_level") == 0) {
            if (strcmp(val, "TRACE") == 0) cfg->log_level = LOG_TRACE;
            else if (strcmp(val, "DEBUG") == 0) cfg->log_level = LOG_DEBUG;
            else if (strcmp(val, "INFO") == 0) cfg->log_level = LOG_INFO;
            else if (strcmp(val, "WARN") == 0) cfg->log_level = LOG_WARN;
            else if (strcmp(val, "ERROR") == 0) cfg->log_level = LOG_ERROR;
            else if (strcmp(val, "FATAL") == 0) cfg->log_level = LOG_FATAL;
        } else if (strcmp(key, "log_file") == 0 && val[0] == '"') {
            char *end = strrchr(val + 1, '"');
            if (end) { *end = '\0'; strncpy(cfg->log_file, val + 1, sizeof(cfg->log_file) - 1); }
        } else if (strcmp(key, "rule_engine_socket") == 0 && val[0] == '"') {
            char *end = strrchr(val + 1, '"');
            if (end) { *end = '\0'; strncpy(cfg->rule_engine_socket, val + 1, sizeof(cfg->rule_engine_socket) - 1); }
        } else if (strcmp(key, "policy_engine_socket") == 0 && val[0] == '"') {
            char *end = strrchr(val + 1, '"');
            if (end) { *end = '\0'; strncpy(cfg->policy_engine_socket, val + 1, sizeof(cfg->policy_engine_socket) - 1); }
        }
    }

    fclose(f);
    log_info(MODULE, "config loaded from %s", path);
    return 0;
}

void config_print(const config_t *cfg) {
    log_info(MODULE, "socket_path: %s", cfg->socket_path);
    log_info(MODULE, "max_connections: %d", cfg->max_connections);
    log_info(MODULE, "request_timeout_ms: %d", cfg->request_timeout_ms);
    log_info(MODULE, "enable_metrics: %s", cfg->enable_metrics ? "true" : "false");
    log_info(MODULE, "rule_engine_socket: %s", cfg->rule_engine_socket);
    log_info(MODULE, "policy_engine_socket: %s", cfg->policy_engine_socket);
}
