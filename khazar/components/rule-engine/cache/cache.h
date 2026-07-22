#ifndef CACHE_H
#define CACHE_H

#include "common.h"

int  cache_init(void);
int  cache_lookup(const char *query, match_result_t *result);
void cache_store(const char *query, const match_result_t *result);
void cache_cleanup(void);
int  cache_size(void);
void cache_clear(void);

#endif
