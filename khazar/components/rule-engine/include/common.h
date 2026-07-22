#ifndef COMMON_H
#define COMMON_H

#include "../../../sdk/include/common.h"
#include <string.h>
#include <regex.h>

#define MAX_CONNECTIONS        256
#define MAX_TOKENS             1024
#define MAX_REGEX_RULES        1024
#define MAX_INTENT_ENTRIES     2048
#define MAX_ALIASES            1024
#define MAX_CACHE_ENTRIES      256
#define MAX_TOKEN_LEN          128
#define MAX_QUERY_LEN          2048
#define MAX_INTENT_NAME_LEN    128
#define MAX_TARGET_LEN         256
#define MAX_ACTION_LEN         128
#define MAX_ARG_LEN            64
#define MAX_ARGS               8
#define MAX_MULTI_RESULTS      8
#define SOCKET_PATH_DEFAULT    "/run/ai-rule-engine.sock"
#define PID_FILE_DEFAULT       "/run/ai-rule-engine.pid"
#define VERSION_MAJOR          0
#define VERSION_MINOR          3
#define VERSION_PATCH          0
#define CACHE_CLEANUP_INTERVAL 300

typedef enum {
    MATCH_SOURCE_CACHE  = 0,
    MATCH_SOURCE_REGEX  = 1,
    MATCH_SOURCE_TOKEN  = 2,
    MATCH_SOURCE_INTENT = 3,
    MATCH_SOURCE_ALIAS  = 4,
    MATCH_SOURCE_FUZZY  = 5,
    MATCH_SOURCE_NONE   = 6
} match_source_t;

typedef enum {
    RULE_SOURCE_BUILTIN = 0,
    RULE_SOURCE_FILE    = 1,
    RULE_SOURCE_RUNTIME = 2
} rule_source_t;

typedef enum {
    INTENT_OPEN_APPLICATION = 0,
    INTENT_CLOSE_APPLICATION,
    INTENT_INSTALL_PACKAGE,
    INTENT_REMOVE_PACKAGE,
    INTENT_SEARCH_PACKAGE,
    INTENT_SYSTEM_UPDATE,
    INTENT_SYSTEM_SHUTDOWN,
    INTENT_SYSTEM_REBOOT,
    INTENT_SYSTEM_SUSPEND,
    INTENT_SYSTEM_LOCK,
    INTENT_SYSTEM_HIBERNATE,
    INTENT_SYSTEM_LOGOUT,
    INTENT_NETWORK_ENABLE,
    INTENT_NETWORK_DISABLE,
    INTENT_NETWORK_STATUS,
    INTENT_VOLUME_UP,
    INTENT_VOLUME_DOWN,
    INTENT_VOLUME_MUTE,
    INTENT_VOLUME_SET,
    INTENT_BRIGHTNESS_UP,
    INTENT_BRIGHTNESS_DOWN,
    INTENT_BRIGHTNESS_SET,
    INTENT_SCREENSHOT,
    INTENT_FILE_OPEN,
    INTENT_FILE_SEARCH,
    INTENT_WINDOW_MINIMIZE,
    INTENT_WINDOW_MAXIMIZE,
    INTENT_NOTIFICATION_SEND,
    INTENT_CLIPBOARD_COPY,
    INTENT_PROCESS_KILL,
    INTENT_UNKNOWN
} intent_type_t;

#define INTENT_COUNT (INTENT_UNKNOWN + 1)

static inline const char *intent_type_str(intent_type_t t) {
    switch (t) {
        case INTENT_OPEN_APPLICATION:   return "open_application";
        case INTENT_CLOSE_APPLICATION:  return "close_application";
        case INTENT_INSTALL_PACKAGE:    return "install_package";
        case INTENT_REMOVE_PACKAGE:     return "remove_package";
        case INTENT_SEARCH_PACKAGE:     return "search_package";
        case INTENT_SYSTEM_UPDATE:      return "system_update";
        case INTENT_SYSTEM_SHUTDOWN:    return "system_shutdown";
        case INTENT_SYSTEM_REBOOT:      return "system_reboot";
        case INTENT_SYSTEM_SUSPEND:     return "system_suspend";
        case INTENT_SYSTEM_LOCK:        return "system_lock";
        case INTENT_SYSTEM_HIBERNATE:   return "system_hibernate";
        case INTENT_SYSTEM_LOGOUT:      return "system_logout";
        case INTENT_NETWORK_ENABLE:     return "network_enable";
        case INTENT_NETWORK_DISABLE:    return "network_disable";
        case INTENT_NETWORK_STATUS:     return "network_status";
        case INTENT_VOLUME_UP:          return "volume_up";
        case INTENT_VOLUME_DOWN:        return "volume_down";
        case INTENT_VOLUME_MUTE:        return "volume_mute";
        case INTENT_VOLUME_SET:         return "volume_set";
        case INTENT_BRIGHTNESS_UP:      return "brightness_up";
        case INTENT_BRIGHTNESS_DOWN:    return "brightness_down";
        case INTENT_BRIGHTNESS_SET:     return "brightness_set";
        case INTENT_SCREENSHOT:         return "screenshot";
        case INTENT_FILE_OPEN:          return "file_open";
        case INTENT_FILE_SEARCH:        return "file_search";
        case INTENT_WINDOW_MINIMIZE:    return "window_minimize";
        case INTENT_WINDOW_MAXIMIZE:    return "window_maximize";
        case INTENT_NOTIFICATION_SEND:  return "notification_send";
        case INTENT_CLIPBOARD_COPY:     return "clipboard_copy";
        case INTENT_PROCESS_KILL:       return "process_kill";
        default:                        return "unknown";
    }
}

static inline intent_type_t intent_type_from_str(const char *s) {
    for (int i = 0; i < INTENT_UNKNOWN; i++) {
        if (strcmp(s, intent_type_str((intent_type_t)i)) == 0) return (intent_type_t)i;
    }
    return INTENT_UNKNOWN;
}

static inline const char *intent_capability(intent_type_t t) {
    switch (t) {
        case INTENT_OPEN_APPLICATION:
        case INTENT_CLOSE_APPLICATION:
        case INTENT_WINDOW_MINIMIZE:
        case INTENT_WINDOW_MAXIMIZE:
            return "open_application";
        case INTENT_INSTALL_PACKAGE:
        case INTENT_REMOVE_PACKAGE:
        case INTENT_SEARCH_PACKAGE:
        case INTENT_SYSTEM_UPDATE:
            return "install_package";
        case INTENT_SYSTEM_SHUTDOWN:
        case INTENT_SYSTEM_REBOOT:
        case INTENT_SYSTEM_SUSPEND:
        case INTENT_SYSTEM_LOCK:
        case INTENT_SYSTEM_HIBERNATE:
        case INTENT_SYSTEM_LOGOUT:
            return "system_management";
        case INTENT_NETWORK_ENABLE:
        case INTENT_NETWORK_DISABLE:
        case INTENT_NETWORK_STATUS:
            return "network_management";
        case INTENT_VOLUME_UP:
        case INTENT_VOLUME_DOWN:
        case INTENT_VOLUME_MUTE:
        case INTENT_VOLUME_SET:
            return "audio_control";
        case INTENT_BRIGHTNESS_UP:
        case INTENT_BRIGHTNESS_DOWN:
        case INTENT_BRIGHTNESS_SET:
            return "display_control";
        case INTENT_SCREENSHOT:
        case INTENT_CLIPBOARD_COPY:
            return "screenshot";
        case INTENT_FILE_OPEN:
        case INTENT_FILE_SEARCH:
            return "file_management";
        case INTENT_NOTIFICATION_SEND:
            return "notification";
        case INTENT_PROCESS_KILL:
            return "system_management";
        default:
            return "unknown";
    }
}

typedef struct {
    char     token[MAX_TOKEN_LEN];
    intent_type_t intent;
    int      weight;
    rule_source_t source;
} token_entry_t;

typedef struct {
    regex_t  regex;
    char     pattern[MAX_TOKEN_LEN];
    intent_type_t intent;
    int      priority;
    rule_source_t source;
} regex_rule_t;

typedef struct {
    char     query[MAX_QUERY_LEN];
    intent_type_t intent;
    char     target[MAX_TARGET_LEN];
    char     action[MAX_ACTION_LEN];
    rule_source_t source;
} intent_entry_t;

typedef struct {
    char     alias[MAX_TOKEN_LEN];
    char     canonical[MAX_TOKEN_LEN];
    rule_source_t source;
} alias_entry_t;

typedef struct {
    char     query[MAX_QUERY_LEN];
    intent_type_t intent;
    char     target[MAX_TARGET_LEN];
    char     action[MAX_ACTION_LEN];
    time_t   timestamp;
    int      hit_count;
} cache_entry_t;

typedef struct {
    char     key[MAX_TOKEN_LEN];
    char     value[MAX_ARG_LEN];
} match_arg_t;

typedef struct {
    intent_type_t intent;
    char     target[MAX_TARGET_LEN];
    char     action[MAX_ACTION_LEN];
    float    confidence;
    match_source_t source;
    char     matched_pattern[MAX_TOKEN_LEN];
    match_arg_t args[MAX_ARGS];
    int      arg_count;
} match_result_t;

typedef struct {
    float    cache;
    float    regex;
    float    token;
    float    intent;
    float    alias;
    float    fuzzy;
} confidence_threshold_t;

typedef struct {
    uint64_t total_requests;
    uint64_t cache_hits;
    uint64_t regex_hits;
    uint64_t token_hits;
    uint64_t intent_hits;
    uint64_t alias_hits;
    uint64_t fuzzy_hits;
    uint64_t unknown_hits;
    uint64_t multi_intents;
    double   avg_latency_us;
    double   max_latency_us;
    uint64_t active_connections;
    uint64_t cache_size;
    uint64_t cache_evictions;
    time_t   uptime_start;
} pipeline_metrics_t;

#endif
