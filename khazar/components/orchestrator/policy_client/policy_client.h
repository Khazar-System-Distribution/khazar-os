#ifndef POLICY_CLIENT_H
#define POLICY_CLIENT_H

#include "../include/common.h"

typedef struct policy_client policy_client_t;

policy_client_t *policy_client_init(const char *socket_path);
int              policy_client_check(policy_client_t *pc, const char *agent_name,
                                     const char *capability, const char *target,
                                     bool *allowed, char *reason, size_t reason_len);
void             policy_client_cleanup(policy_client_t *pc);

#endif
