#ifndef AI_SDK_AGENT_H
#define AI_SDK_AGENT_H

#include "../include/common.h"
#include "../ipc/ipc.h"

typedef int (*agent_task_handler_t)(const request_t *req, response_t *resp, void *ctx);

typedef struct {
    const char            *name;
    const char            *version;
    const char            *capabilities[MAX_CAPABILITIES];
    int                    cap_count;
    const char            *orchestrator_socket;
    const char            *agent_socket_path;
    agent_task_handler_t   task_handler;
    void                  *handler_ctx;
} agent_config_t;

typedef struct agent agent_t;

agent_t     *agent_init(const agent_config_t *config);
int          agent_start(agent_t *a);
int          agent_register(agent_t *a);
int          agent_run(agent_t *a);
void         agent_stop(agent_t *a);
void         agent_cleanup(agent_t *a);

int          agent_heartbeat(agent_t *a);
agent_state_t agent_get_state(agent_t *a);
const char  *agent_get_name(agent_t *a);

int          agent_send_response(agent_t *a, int client_fd, const response_t *resp);

#endif
