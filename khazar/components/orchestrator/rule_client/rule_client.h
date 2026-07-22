#ifndef RULE_CLIENT_H
#define RULE_CLIENT_H

#include "../include/common.h"
#include "../router/router.h"

typedef struct rule_client rule_client_t;

rule_client_t *rule_client_init(const char *socket_path);
int            rule_client_query(rule_client_t *rc, const char *query, intent_t *intent, float *confidence);
void           rule_client_cleanup(rule_client_t *rc);

#endif
