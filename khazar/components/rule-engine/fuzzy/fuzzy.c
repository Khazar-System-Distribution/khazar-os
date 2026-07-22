#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "fuzzy/fuzzy.h"
#include "logger/logger.h"

static intent_entry_t g_intents[MAX_INTENT_ENTRIES];
static int            g_intent_count = 0;
static float          g_threshold = 0.70f;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int fuzzy_init(void) {
    memset(g_intents, 0, sizeof(g_intents));
    g_intent_count = 0;
    return 0;
}

int fuzzy_set_threshold(float threshold) {
    g_threshold = threshold;
    return 0;
}

static int levenshtein(const char *s1, const char *s2) {
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);
    int matrix[128][128];

    if (len1 >= 127) len1 = 126;
    if (len2 >= 127) len2 = 126;

    for (int i = 0; i <= len1; i++) matrix[i][0] = i;
    for (int j = 0; j <= len2; j++) matrix[0][j] = j;

    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            int del = matrix[i - 1][j] + 1;
            int ins = matrix[i][j - 1] + 1;
            int sub = matrix[i - 1][j - 1] + cost;
            int min_val = del;
            if (ins < min_val) min_val = ins;
            if (sub < min_val) min_val = sub;
            matrix[i][j] = min_val;
        }
    }
    return matrix[len1][len2];
}

static float similarity(const char *a, const char *b) {
    int max_len = (int)strlen(a);
    if ((int)strlen(b) > max_len) max_len = (int)strlen(b);
    if (max_len == 0) return 1.0f;
    int dist = levenshtein(a, b);
    return 1.0f - (float)dist / (float)max_len;
}

int fuzzy_match(const char *query, match_result_t *result) {
    char query_lower[MAX_QUERY_LEN];
    float best_sim = 0.0f;
    int best_idx = -1;

    if (!query || !result) return -1;

    memset(result, 0, sizeof(*result));
    result->intent = INTENT_UNKNOWN;

    strncpy(query_lower, query, sizeof(query_lower) - 1);
    for (char *p = query_lower; *p; p++) *p = (char)tolower((unsigned char)*p);

    pthread_mutex_lock(&g_mutex);

    for (int i = 0; i < g_intent_count; i++) {
        char entry_lower[MAX_QUERY_LEN];
        strncpy(entry_lower, g_intents[i].query, sizeof(entry_lower) - 1);
        for (char *p = entry_lower; *p; p++) *p = (char)tolower((unsigned char)*p);

        float sim = similarity(query_lower, entry_lower);
        if (sim > best_sim && sim >= g_threshold) {
            best_sim = sim;
            best_idx = i;
        }
    }

    if (best_idx >= 0) {
        result->intent = g_intents[best_idx].intent;
        strncpy(result->target, g_intents[best_idx].target, MAX_TARGET_LEN - 1);
        strncpy(result->action, g_intents[best_idx].action, MAX_ACTION_LEN - 1);
        strncpy(result->matched_pattern, g_intents[best_idx].query, MAX_TOKEN_LEN - 1);
        result->confidence = best_sim;
        result->source = MATCH_SOURCE_FUZZY;
    }

    pthread_mutex_unlock(&g_mutex);

    return (best_idx >= 0) ? 0 : -1;
}

void fuzzy_cleanup(void) {
    pthread_mutex_lock(&g_mutex);
    memset(g_intents, 0, sizeof(g_intents));
    g_intent_count = 0;
    pthread_mutex_unlock(&g_mutex);
}
