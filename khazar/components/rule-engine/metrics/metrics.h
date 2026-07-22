#ifndef METRICS_H
#define METRICS_H

#include "common.h"

int  metrics_init(void);
void metrics_record_request(void);
void metrics_record_hit(match_source_t source);
void metrics_record_unknown(void);
void metrics_record_multi(int count);
void metrics_record_latency(double us);
void metrics_set_connections(uint64_t n);
void metrics_set_cache_size(uint64_t n);
void metrics_set_cache_evictions(uint64_t n);
void metrics_get_snapshot(pipeline_metrics_t *m);
void metrics_cleanup(void);

#endif
