#ifndef ROUTER_H
#define ROUTER_H

#include "../include/common.h"
#include "../registry/registry.h"

typedef struct router router_t;

router_t *router_init(registry_t *reg);
int       router_resolve(router_t *rt, const request_t *req, intent_t *intent);
void      router_cleanup(router_t *rt);

#endif
