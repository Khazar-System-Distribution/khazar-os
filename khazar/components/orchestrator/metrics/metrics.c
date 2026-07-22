#include "metrics.h"
#include "logger/logger.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define MODULE "metrics"

struct metrics {
    bool enabled;
    uint64_t requests_total;
    uint64_t requests_success;
    uint64_t requests_failed;
    uint64_t requests_timeout;
    uint64_t policy_denied;
    uint64_t agent_failures;
    uint64_t current_queue_size;
    uint64_t active_connections;
    double   total_latency_ms;
    double   max_latency_ms;
    uint64_t latency_count;
    pthread_mutex_t mutex;
};

metrics_t *metrics_init(bool enabled) {
    metrics_t *m = calloc(1, sizeof(metrics_t));
    if (!m) return NULL;

    m->enabled = enabled;
    pthread_mutex_init(&m->mutex, NULL);

    if (enabled) {
        log_info(MODULE, "metrics collection enabled");
    }

    return m;
}

void metrics_record_request(metrics_t *m, double latency_ms, bool success) {
    if (!m || !m->enabled) return;

    pthread_mutex_lock(&m->mutex);
    m->requests_total++;
    if (success) m->requests_success++; else m->requests_failed++;
    m->total_latency_ms += latency_ms;
    m->latency_count++;
    if (latency_ms > m->max_latency_ms) m->max_latency_ms = latency_ms;
    pthread_mutex_unlock(&m->mutex);
}

void metrics_record_timeout(metrics_t *m) {
    if (!m || !m->enabled) return;
    pthread_mutex_lock(&m->mutex);
    m->requests_timeout++;
    m->requests_failed++;
    pthread_mutex_unlock(&m->mutex);
}

void metrics_record_policy_denied(metrics_t *m) {
    if (!m || !m->enabled) return;
    pthread_mutex_lock(&m->mutex);
    m->policy_denied++;
    pthread_mutex_unlock(&m->mutex);
}

void metrics_record_agent_failure(metrics_t *m) {
    if (!m || !m->enabled) return;
    pthread_mutex_lock(&m->mutex);
    m->agent_failures++;
    pthread_mutex_unlock(&m->mutex);
}

void metrics_set_queue_size(metrics_t *m, uint64_t size) {
    if (!m || !m->enabled) return;
    pthread_mutex_lock(&m->mutex);
    m->current_queue_size = size;
    pthread_mutex_unlock(&m->mutex);
}

void metrics_set_active_connections(metrics_t *m, uint64_t count) {
    if (!m || !m->enabled) return;
    pthread_mutex_lock(&m->mutex);
    m->active_connections = count;
    pthread_mutex_unlock(&m->mutex);
}

void metrics_snapshot(metrics_t *m, metrics_snapshot_t *snap) {
    if (!m || !snap) return;

    pthread_mutex_lock(&m->mutex);
    snap->requests_total = m->requests_total;
    snap->requests_success = m->requests_success;
    snap->requests_failed = m->requests_failed;
    snap->requests_timeout = m->requests_timeout;
    snap->policy_denied = m->policy_denied;
    snap->agent_failures = m->agent_failures;
    snap->current_queue_size = m->current_queue_size;
    snap->active_connections = m->active_connections;
    snap->avg_latency_ms = m->latency_count > 0 ? m->total_latency_ms / (double)m->latency_count : 0.0;
    snap->max_latency_ms = m->max_latency_ms;
    pthread_mutex_unlock(&m->mutex);
}

void metrics_cleanup(metrics_t *m) {
    if (!m) return;
    pthread_mutex_destroy(&m->mutex);
    free(m);
    log_info(MODULE, "metrics cleaned up");
}
