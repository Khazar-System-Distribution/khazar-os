#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "cache/cache.h"
#include "logger/logger.h"

static cache_entry_t   g_cache[MAX_CACHE_ENTRIES];
static int             g_cache_count = 0;
static int             g_lru_counter = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int cache_init(void) {
    memset(g_cache, 0, sizeof(g_cache));
    g_cache_count = 0;
    g_lru_counter = 0;
    return 0;
}

static unsigned int hash_query(const char *query) {
    unsigned int hash = 5381;
    int c;
    while ((c = *query++)) {
        hash = ((hash << 5) + hash) + (unsigned int)c;
    }
    return hash % MAX_CACHE_ENTRIES;
}

int cache_lookup(const char *query, match_result_t *result) {
    unsigned int hash;

    if (!query || !result) return -1;

    memset(result, 0, sizeof(*result));

    pthread_mutex_lock(&g_mutex);

    hash = hash_query(query);

    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        int idx = (int)((hash + (unsigned int)i) % (unsigned int)MAX_CACHE_ENTRIES);
        if (g_cache[idx].query[0] == '\0') continue;

        if (strcmp(g_cache[idx].query, query) == 0) {
            result->intent = g_cache[idx].intent;
            strncpy(result->target, g_cache[idx].target, MAX_TARGET_LEN - 1);
            strncpy(result->action, g_cache[idx].action, MAX_ACTION_LEN - 1);
            result->confidence = 1.0f;
            result->source = MATCH_SOURCE_CACHE;

            g_cache[idx].hit_count++;
            g_cache[idx].timestamp = time(NULL);
            g_cache[idx].timestamp += g_lru_counter / 256;

            pthread_mutex_unlock(&g_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_mutex);
    return -1;
}

void cache_store(const char *query, const match_result_t *result) {
    unsigned int hash;
    int oldest_idx = 0;
    time_t oldest_time = time(NULL) + 1;

    if (!query || !result || result->intent == INTENT_UNKNOWN) return;

    pthread_mutex_lock(&g_mutex);

    hash = hash_query(query);

    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        int idx = (int)((hash + (unsigned int)i) % (unsigned int)MAX_CACHE_ENTRIES);

        if (g_cache[idx].query[0] == '\0') {
            strncpy(g_cache[idx].query, query, MAX_QUERY_LEN - 1);
            g_cache[idx].intent = result->intent;
            strncpy(g_cache[idx].target, result->target, MAX_TARGET_LEN - 1);
            strncpy(g_cache[idx].action, result->action, MAX_ACTION_LEN - 1);
            g_cache[idx].timestamp = time(NULL) + (g_lru_counter / 256);
            g_cache[idx].hit_count = 1;
            g_cache_count++;
            g_lru_counter++;
            pthread_mutex_unlock(&g_mutex);
            return;
        }

        if (strcmp(g_cache[idx].query, query) == 0) {
            g_cache[idx].hit_count++;
            g_cache[idx].timestamp = time(NULL) + (g_lru_counter / 256);
            g_lru_counter++;
            pthread_mutex_unlock(&g_mutex);
            return;
        }
    }

    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        if (g_cache[i].timestamp < oldest_time) {
            oldest_time = g_cache[i].timestamp;
            oldest_idx = i;
        }
    }

    strncpy(g_cache[oldest_idx].query, query, MAX_QUERY_LEN - 1);
    g_cache[oldest_idx].intent = result->intent;
    strncpy(g_cache[oldest_idx].target, result->target, MAX_TARGET_LEN - 1);
    strncpy(g_cache[oldest_idx].action, result->action, MAX_ACTION_LEN - 1);
    g_cache[oldest_idx].timestamp = time(NULL) + (g_lru_counter / 256);
    g_cache[oldest_idx].hit_count = 1;
    g_lru_counter++;

    pthread_mutex_unlock(&g_mutex);
}

void cache_cleanup(void) {
    pthread_mutex_lock(&g_mutex);
    memset(g_cache, 0, sizeof(g_cache));
    g_cache_count = 0;
    pthread_mutex_unlock(&g_mutex);
}

int cache_size(void) {
    return g_cache_count;
}

void cache_clear(void) {
    pthread_mutex_lock(&g_mutex);
    memset(g_cache, 0, sizeof(g_cache));
    g_cache_count = 0;
    g_lru_counter = 0;
    pthread_mutex_unlock(&g_mutex);
}
