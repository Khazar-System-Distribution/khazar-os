#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "../include/common.h"
#include "config/config.h"

static config_t g_config;
static pthread_mutex_t g_config_mutex = PTHREAD_MUTEX_INITIALIZER;

config_t *config_get(void) { return &g_config; }

void config_set_defaults(void) {
    pthread_mutex_lock(&g_config_mutex);
    strncpy(g_config.socket_path, SOCKET_PATH_DEFAULT, sizeof(g_config.socket_path) - 1);
    g_config.max_connections = MAX_CONNECTIONS;
    g_config.request_timeout_ms = REQUEST_TIMEOUT_MS;
    g_config.enable_cache = true;
    g_config.enable_regex = true;
    g_config.enable_tokens = true;
    g_config.enable_intent_table = true;
    g_config.enable_alias = true;
    g_config.enable_fuzzy = true;
    g_config.daemonize = false;
    strncpy(g_config.pid_file, PID_FILE_DEFAULT, sizeof(g_config.pid_file) - 1);
    g_config.log_file[0] = '\0';
    g_config.rules_dir[0] = '\0';
    g_config.log_level = LOG_INFO;
    g_config.cache_cleanup_interval = CACHE_CLEANUP_INTERVAL;
    g_config.fuzzy_threshold = 0.70f;
    g_config.cache_confidence_threshold = 0.99f;
    g_config.regex_confidence_threshold = 0.85f;
    g_config.token_confidence_threshold = 0.50f;
    g_config.intent_confidence_threshold = 0.99f;
    g_config.alias_confidence_threshold = 0.75f;
    pthread_mutex_unlock(&g_config_mutex);
}

static void trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
}

int config_load(const char *path) {
    FILE *f;
    char line[512], key[256], value[256];

    config_set_defaults();
    if (!path) return 0;

    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "config: fayl tapilmadi '%s', default-lar istifade olunur\n", path);
        return 0;
    }

    pthread_mutex_lock(&g_config_mutex);
    while (fgets(line, sizeof(line), f)) {
        char *eq;
        trim_whitespace(line);
        if (line[0] == '#' || line[0] == '\0' || line[0] == '[') continue;
        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        strncpy(key, line, sizeof(key) - 1); key[sizeof(key) - 1] = '\0';
        strncpy(value, eq + 1, sizeof(value) - 1); value[sizeof(value) - 1] = '\0';
        trim_whitespace(key); trim_whitespace(value);
        if (value[0] == '"') {
            memmove(value, value + 1, strlen(value));
            char *cp = strrchr(value, '"'); if (cp) *cp = '\0';
        }

        if (strcmp(key, "socket_path") == 0) strncpy(g_config.socket_path, value, sizeof(g_config.socket_path) - 1);
        else if (strcmp(key, "max_connections") == 0) g_config.max_connections = atoi(value);
        else if (strcmp(key, "request_timeout_ms") == 0) g_config.request_timeout_ms = atoi(value);
        else if (strcmp(key, "enable_cache") == 0) g_config.enable_cache = (strcmp(value, "true") == 0);
        else if (strcmp(key, "enable_regex") == 0) g_config.enable_regex = (strcmp(value, "true") == 0);
        else if (strcmp(key, "enable_tokens") == 0) g_config.enable_tokens = (strcmp(value, "true") == 0);
        else if (strcmp(key, "enable_intent_table") == 0) g_config.enable_intent_table = (strcmp(value, "true") == 0);
        else if (strcmp(key, "enable_alias") == 0) g_config.enable_alias = (strcmp(value, "true") == 0);
        else if (strcmp(key, "enable_fuzzy") == 0) g_config.enable_fuzzy = (strcmp(value, "true") == 0);
        else if (strcmp(key, "daemonize") == 0) g_config.daemonize = (strcmp(value, "true") == 0);
        else if (strcmp(key, "pid_file") == 0) strncpy(g_config.pid_file, value, sizeof(g_config.pid_file) - 1);
        else if (strcmp(key, "log_file") == 0) strncpy(g_config.log_file, value, sizeof(g_config.log_file) - 1);
        else if (strcmp(key, "rules_dir") == 0) strncpy(g_config.rules_dir, value, sizeof(g_config.rules_dir) - 1);
        else if (strcmp(key, "log_level") == 0) g_config.log_level = atoi(value);
        else if (strcmp(key, "cache_cleanup_interval") == 0) g_config.cache_cleanup_interval = atoi(value);
        else if (strcmp(key, "fuzzy_threshold") == 0) g_config.fuzzy_threshold = (float)atof(value);
        else if (strcmp(key, "cache_confidence_threshold") == 0) g_config.cache_confidence_threshold = (float)atof(value);
        else if (strcmp(key, "regex_confidence_threshold") == 0) g_config.regex_confidence_threshold = (float)atof(value);
        else if (strcmp(key, "token_confidence_threshold") == 0) g_config.token_confidence_threshold = (float)atof(value);
        else if (strcmp(key, "intent_confidence_threshold") == 0) g_config.intent_confidence_threshold = (float)atof(value);
        else if (strcmp(key, "alias_confidence_threshold") == 0) g_config.alias_confidence_threshold = (float)atof(value);
    }
    fclose(f);
    pthread_mutex_unlock(&g_config_mutex);
    return 0;
}
