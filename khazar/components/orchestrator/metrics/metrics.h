#ifndef METRICS_H
#define METRICS_H

#include "../include/common.h"

typedef struct {
    uint64_t requests_total;
    uint64_t requests_success;
    uint64_t requests_failed;
    uint64_t requests_timeout;
    uint64_t policy_denied;
    double   avg_latency_ms;
    double   max_latency_ms;
    uint64_t current_queue_size;
    uint64_t agent_failures;
    uint64_t active_connections;
} metrics_snapshot_t;

typedef struct metrics metrics_t;

metrics_t *metrics_init(bool enabled);
void       metrics_record_request(metrics_t *m, double latency_ms, bool success);
void       metrics_record_timeout(metrics_t *m);
void       metrics_record_policy_denied(metrics_t *m);
void       metrics_record_agent_failure(metrics_t *m);
void       metrics_set_queue_size(metrics_t *m, uint64_t size);
void       metrics_set_active_connections(metrics_t *m, uint64_t count);
void       metrics_snapshot(metrics_t *m, metrics_snapshot_t *snap);
void       metrics_cleanup(metrics_t *m);

#endif
