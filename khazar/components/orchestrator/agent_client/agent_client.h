#ifndef AGENT_CLIENT_H
#define AGENT_CLIENT_H

#include "../include/common.h"
#include "../registry/registry.h"

typedef struct agent_client agent_client_t;

agent_client_t *agent_client_init(void);
int             agent_client_dispatch(agent_client_t *ac, const agent_info_t *agent,
                                      const request_t *req, response_t *resp);
void            agent_client_cleanup(agent_client_t *ac);

#endif
