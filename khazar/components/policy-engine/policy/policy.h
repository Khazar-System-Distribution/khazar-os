#ifndef POLICY_H
#define POLICY_H

#include "../include/common.h"

typedef struct policy_store policy_store_t;

policy_store_t *policy_init(void);
int             policy_load_file(policy_store_t *ps, const char *path);
int             policy_load_dir(policy_store_t *ps, const char *dir);
int             policy_add_rule(policy_store_t *ps, const policy_rule_t *rule);
int             policy_check(policy_store_t *ps, const policy_request_t *req, policy_response_t *resp);
int             policy_reload(policy_store_t *ps, const char *dir);
void            policy_cleanup(policy_store_t *ps);

#endif
