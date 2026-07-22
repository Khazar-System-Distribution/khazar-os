#ifndef REGISTRY_H
#define REGISTRY_H

#include "../include/common.h"

typedef struct registry registry_t;

registry_t *registry_init(void);
int         registry_register(registry_t *reg, const agent_info_t *agent);
int         registry_unregister(registry_t *reg, const char *name);
agent_info_t *registry_find_by_capability(registry_t *reg, const char *capability);
agent_info_t *registry_find_by_name(registry_t *reg, const char *name);
void        registry_cleanup(registry_t *reg);

#endif
