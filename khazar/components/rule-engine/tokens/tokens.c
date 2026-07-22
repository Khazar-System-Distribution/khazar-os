#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "tokens/tokens.h"
#include "logger/logger.h"

static token_entry_t g_tokens[MAX_TOKENS];
static int           g_token_count = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int tokens_init(void) {
    memset(g_tokens, 0, sizeof(g_tokens));
    g_token_count = 0;
    return 0;
}

int tokens_add(const char *token, intent_type_t intent, int weight, rule_source_t source) {
    if (g_token_count >= MAX_TOKENS) return -1;
    pthread_mutex_lock(&g_mutex);
    strncpy(g_tokens[g_token_count].token, token, MAX_TOKEN_LEN - 1);
    g_tokens[g_token_count].intent = intent;
    g_tokens[g_token_count].weight = weight;
    g_tokens[g_token_count].source = source;
    g_token_count++;
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

static void to_lowercase(char *dst, const char *src, size_t max) {
    size_t i = 0;
    while (src[i] && i < max - 1) { dst[i] = (char)tolower((unsigned char)src[i]); i++; }
    dst[i] = '\0';
}

int tokens_lookup(const char *query, match_result_t *result) {
    char query_lower[MAX_QUERY_LEN];
    int best_idx = -1, best_score = 0;

    if (!query || !result) return -1;
    memset(result, 0, sizeof(*result));
    result->intent = INTENT_UNKNOWN;
    to_lowercase(query_lower, query, sizeof(query_lower));

    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < g_token_count; i++) {
        char token_lower[MAX_TOKEN_LEN];
        to_lowercase(token_lower, g_tokens[i].token, sizeof(token_lower));
        if (strstr(query_lower, token_lower)) {
            int score = g_tokens[i].weight + (int)strlen(token_lower);
            if (score > best_score) { best_score = score; best_idx = i; }
        }
    }

    if (best_idx >= 0) {
        result->intent = g_tokens[best_idx].intent;
        strncpy(result->action, g_tokens[best_idx].token, MAX_ACTION_LEN - 1);
        strncpy(result->matched_pattern, g_tokens[best_idx].token, MAX_TOKEN_LEN - 1);
        result->confidence = (float)best_score / (float)(best_score + 10);
        if (result->confidence > 1.0f) result->confidence = 1.0f;
        result->source = MATCH_SOURCE_TOKEN;
        if (result->target[0] == '\0') strncpy(result->target, query, MAX_TARGET_LEN - 1);
    }
    pthread_mutex_unlock(&g_mutex);
    return (best_idx >= 0) ? 0 : -1;
}

void tokens_clear_rules(rule_source_t source) {
    pthread_mutex_lock(&g_mutex);
    int write = 0;
    for (int i = 0; i < g_token_count; i++) {
        if (g_tokens[i].source != source) {
            if (i != write) g_tokens[write] = g_tokens[i];
            write++;
        }
    }
    g_token_count = write;
    pthread_mutex_unlock(&g_mutex);
}

int tokens_load_defaults(void) {
    tokens_add("aç",         INTENT_OPEN_APPLICATION,  5, RULE_SOURCE_BUILTIN);
    tokens_add("ac",         INTENT_OPEN_APPLICATION,  5, RULE_SOURCE_BUILTIN);
    tokens_add("open",       INTENT_OPEN_APPLICATION,  5, RULE_SOURCE_BUILTIN);
    tokens_add("start",      INTENT_OPEN_APPLICATION,  4, RULE_SOURCE_BUILTIN);
    tokens_add("başlat",     INTENT_OPEN_APPLICATION,  4, RULE_SOURCE_BUILTIN);
    tokens_add("launch",     INTENT_OPEN_APPLICATION,  4, RULE_SOURCE_BUILTIN);
    tokens_add("bağla",      INTENT_CLOSE_APPLICATION, 5, RULE_SOURCE_BUILTIN);
    tokens_add("close",      INTENT_CLOSE_APPLICATION, 5, RULE_SOURCE_BUILTIN);
    tokens_add("kapat",      INTENT_CLOSE_APPLICATION, 5, RULE_SOURCE_BUILTIN);
    tokens_add("söndür",     INTENT_CLOSE_APPLICATION, 4, RULE_SOURCE_BUILTIN);
    tokens_add("kill",       INTENT_CLOSE_APPLICATION, 4, RULE_SOURCE_BUILTIN);
    tokens_add("quraşdır",   INTENT_INSTALL_PACKAGE,   5, RULE_SOURCE_BUILTIN);
    tokens_add("install",    INTENT_INSTALL_PACKAGE,   5, RULE_SOURCE_BUILTIN);
    tokens_add("yüklə",      INTENT_INSTALL_PACKAGE,   4, RULE_SOURCE_BUILTIN);
    tokens_add("sil",        INTENT_REMOVE_PACKAGE,    5, RULE_SOURCE_BUILTIN);
    tokens_add("remove",     INTENT_REMOVE_PACKAGE,    5, RULE_SOURCE_BUILTIN);
    tokens_add("kaldır",     INTENT_REMOVE_PACKAGE,    4, RULE_SOURCE_BUILTIN);
    tokens_add("ara",        INTENT_SEARCH_PACKAGE,    5, RULE_SOURCE_BUILTIN);
    tokens_add("axtar",      INTENT_SEARCH_PACKAGE,    5, RULE_SOURCE_BUILTIN);
    tokens_add("search",     INTENT_SEARCH_PACKAGE,    5, RULE_SOURCE_BUILTIN);
    tokens_add("yenilə",     INTENT_SYSTEM_UPDATE,     5, RULE_SOURCE_BUILTIN);
    tokens_add("update",     INTENT_SYSTEM_UPDATE,     5, RULE_SOURCE_BUILTIN);
    tokens_add("güncəllə",   INTENT_SYSTEM_UPDATE,     5, RULE_SOURCE_BUILTIN);
    tokens_add("shutdown",   INTENT_SYSTEM_SHUTDOWN,   5, RULE_SOURCE_BUILTIN);
    tokens_add("reboot",     INTENT_SYSTEM_REBOOT,     5, RULE_SOURCE_BUILTIN);
    tokens_add("restart",    INTENT_SYSTEM_REBOOT,     4, RULE_SOURCE_BUILTIN);
    tokens_add("suspend",    INTENT_SYSTEM_SUSPEND,    5, RULE_SOURCE_BUILTIN);
    tokens_add("sleep",      INTENT_SYSTEM_SUSPEND,    4, RULE_SOURCE_BUILTIN);
    tokens_add("lock",       INTENT_SYSTEM_LOCK,       5, RULE_SOURCE_BUILTIN);
    tokens_add("kilidlə",    INTENT_SYSTEM_LOCK,       5, RULE_SOURCE_BUILTIN);
    tokens_add("hibernate",  INTENT_SYSTEM_HIBERNATE,  5, RULE_SOURCE_BUILTIN);
    tokens_add("wifi",       INTENT_NETWORK_ENABLE,    5, RULE_SOURCE_BUILTIN);
    tokens_add("internet",   INTENT_NETWORK_ENABLE,    4, RULE_SOURCE_BUILTIN);
    tokens_add("bluetooth",  INTENT_NETWORK_ENABLE,    4, RULE_SOURCE_BUILTIN);
    tokens_add("network",    INTENT_NETWORK_ENABLE,    4, RULE_SOURCE_BUILTIN);
    tokens_add("səs",        INTENT_VOLUME_UP,         5, RULE_SOURCE_BUILTIN);
    tokens_add("volume",     INTENT_VOLUME_UP,         5, RULE_SOURCE_BUILTIN);
    tokens_add("mute",       INTENT_VOLUME_MUTE,       5, RULE_SOURCE_BUILTIN);
    tokens_add("parlaqlıq",  INTENT_BRIGHTNESS_UP,     5, RULE_SOURCE_BUILTIN);
    tokens_add("brightness", INTENT_BRIGHTNESS_UP,     5, RULE_SOURCE_BUILTIN);
    tokens_add("screenshot", INTENT_SCREENSHOT,        5, RULE_SOURCE_BUILTIN);
    tokens_add("ekran",      INTENT_SCREENSHOT,        4, RULE_SOURCE_BUILTIN);

    log_info("tokens", "%d token yuklendi", g_token_count);
    return 0;
}

void tokens_cleanup(void) {
    pthread_mutex_lock(&g_mutex);
    memset(g_tokens, 0, sizeof(g_tokens));
    g_token_count = 0;
    pthread_mutex_unlock(&g_mutex);
}
