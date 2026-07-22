#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct {
    char     socket_path[256];
    int      max_connections;
    int      request_timeout_ms;
    bool     enable_cache;
    bool     enable_regex;
    bool     enable_tokens;
    bool     enable_intent_table;
    bool     enable_alias;
    bool     enable_fuzzy;
    bool     daemonize;
    char     pid_file[256];
    char     log_file[256];
    char     rules_dir[256];
    int      log_level;
    int      cache_cleanup_interval;
    float    fuzzy_threshold;
    float    cache_confidence_threshold;
    float    regex_confidence_threshold;
    float    token_confidence_threshold;
    float    intent_confidence_threshold;
    float    alias_confidence_threshold;
} config_t;

config_t *config_get(void);
int       config_load(const char *path);
void      config_set_defaults(void);

#endif
