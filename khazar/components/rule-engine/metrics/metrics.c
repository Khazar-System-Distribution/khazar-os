#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "metrics/metrics.h"

static pipeline_metrics_t g_metrics;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int metrics_init(void) {
    pthread_mutex_lock(&g_mutex);
    memset(&g_metrics, 0, sizeof(g_metrics));
    g_metrics.uptime_start = time(NULL);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

void metrics_record_request(void) {
    pthread_mutex_lock(&g_mutex);
    g_metrics.total_requests++;
    pthread_mutex_unlock(&g_mutex);
}

void metrics_record_hit(match_source_t source) {
    pthread_mutex_lock(&g_mutex);
    switch (source) {
        case MATCH_SOURCE_CACHE:  g_metrics.cache_hits++;  break;
        case MATCH_SOURCE_REGEX:  g_metrics.regex_hits++;  break;
        case MATCH_SOURCE_TOKEN:  g_metrics.token_hits++;  break;
        case MATCH_SOURCE_INTENT: g_metrics.intent_hits++; break;
        case MATCH_SOURCE_ALIAS:  g_metrics.alias_hits++;  break;
        case MATCH_SOURCE_FUZZY:  g_metrics.fuzzy_hits++;  break;
        default: break;
    }
    pthread_mutex_unlock(&g_mutex);
}

void metrics_record_unknown(void) {
    pthread_mutex_lock(&g_mutex);
    g_metrics.unknown_hits++;
    pthread_mutex_unlock(&g_mutex);
}

void metrics_record_multi(int count) {
    pthread_mutex_lock(&g_mutex);
    g_metrics.multi_intents++;
    pthread_mutex_unlock(&g_mutex);
    (void)count;
}

void metrics_record_latency(double us) {
    pthread_mutex_lock(&g_mutex);
    if (g_metrics.total_requests <= 1) {
        g_metrics.avg_latency_us = us;
    } else {
        g_metrics.avg_latency_us = (g_metrics.avg_latency_us * 0.95) + (us * 0.05);
    }
    if (us > g_metrics.max_latency_us) g_metrics.max_latency_us = us;
    pthread_mutex_unlock(&g_mutex);
}

void metrics_set_connections(uint64_t n) {
    pthread_mutex_lock(&g_mutex);
    g_metrics.active_connections = n;
    pthread_mutex_unlock(&g_mutex);
}

void metrics_set_cache_size(uint64_t n) {
    pthread_mutex_lock(&g_mutex);
    g_metrics.cache_size = n;
    pthread_mutex_unlock(&g_mutex);
}

void metrics_set_cache_evictions(uint64_t n) {
    pthread_mutex_lock(&g_mutex);
    g_metrics.cache_evictions = n;
    pthread_mutex_unlock(&g_mutex);
}

void metrics_get_snapshot(pipeline_metrics_t *m) {
    if (!m) return;
    pthread_mutex_lock(&g_mutex);
    memcpy(m, &g_metrics, sizeof(pipeline_metrics_t));
    pthread_mutex_unlock(&g_mutex);
}

void metrics_cleanup(void) {
    pthread_mutex_lock(&g_mutex);
    memset(&g_metrics, 0, sizeof(g_metrics));
    pthread_mutex_unlock(&g_mutex);
}
