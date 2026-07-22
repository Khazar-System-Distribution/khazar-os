#include "agent.h"
#include "../protocol/protocol.h"
#include "../logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define MODULE "agent"

struct agent {
    agent_config_t       config;
    agent_state_t        state;
    ipc_client_t        *orch_client;
    ipc_server_t        *task_server;
    bool                 running;
    pthread_t            heartbeat_thread;
    pthread_mutex_t      mutex;
};

static void *heartbeat_loop(void *arg) {
    agent_t *a = (agent_t *)arg;
    char beat_msg[256];

    while (a->running) {
        sleep(HEARTBEAT_INTERVAL_SEC);

        pthread_mutex_lock(&a->mutex);
        bool connected = ipc_client_is_connected(a->orch_client);
        pthread_mutex_unlock(&a->mutex);

        if (!connected) {
            pthread_mutex_lock(&a->mutex);
            if (ipc_client_connect(a->orch_client) == 0) {
                log_info(MODULE, "reconnected to orchestrator");
            }
            pthread_mutex_unlock(&a->mutex);
            continue;
        }

        protocol_build_heartbeat(a->config.name, beat_msg, sizeof(beat_msg));

        pthread_mutex_lock(&a->mutex);
        ipc_client_send(a->orch_client, beat_msg, strlen(beat_msg));
        pthread_mutex_unlock(&a->mutex);
    }

    return NULL;
}

static void task_handler(int client_fd, const char *data, size_t len, void *ctx) {
    agent_t *a = (agent_t *)ctx;
    (void)len;

    message_type_t type;
    if (protocol_parse_type(data, len, &type) < 0) {
        response_t err_resp = {.id = 0, .status = STATUS_ERROR,
                               .error_code = ERR_PROTOCOL_ERROR};
        snprintf(err_resp.payload, sizeof(err_resp.payload), "unknown message type");
        agent_send_response(a, client_fd, &err_resp);
        return;
    }

    switch (type) {
        case MSG_PING: {
            char pong[64];
            protocol_build_ping(pong, sizeof(pong));
            ipc_server_send(client_fd, pong, strlen(pong));
            break;
        }

        case MSG_REQUEST: {
            request_t req;
            memset(&req, 0, sizeof(req));
            protocol_parse_request(data, len, &req);

            response_t resp;
            memset(&resp, 0, sizeof(resp));
            resp.id = req.id;
            resp.status = STATUS_SUCCESS;

            if (a->config.task_handler) {
                int ret = a->config.task_handler(&req, &resp, a->config.handler_ctx);
                if (ret < 0) {
                    resp.status = STATUS_ERROR;
                    resp.error_code = ERR_INTERNAL;
                    snprintf(resp.payload, sizeof(resp.payload),
                             "task handler returned error");
                }
            } else {
                resp.status = STATUS_ERROR;
                resp.error_code = ERR_INTERNAL;
                snprintf(resp.payload, sizeof(resp.payload),
                         "no task handler configured");
            }

            agent_send_response(a, client_fd, &resp);
            break;
        }

        default: {
            response_t err_resp = {.id = 0, .status = STATUS_ERROR,
                                   .error_code = ERR_INVALID_REQUEST};
            snprintf(err_resp.payload, sizeof(err_resp.payload),
                     "unsupported message type");
            agent_send_response(a, client_fd, &err_resp);
            break;
        }
    }
}

agent_t *agent_init(const agent_config_t *config) {
    if (!config || !config->name || !config->version) return NULL;

    agent_t *a = calloc(1, sizeof(agent_t));
    if (!a) return NULL;

    a->config.name = config->name;
    a->config.version = config->version;
    a->config.cap_count = config->cap_count;
    for (int i = 0; i < config->cap_count && i < MAX_CAPABILITIES; i++) {
        a->config.capabilities[i] = config->capabilities[i];
    }
    a->config.orchestrator_socket = config->orchestrator_socket
        ? config->orchestrator_socket : DEFAULT_ORCHESTRATOR_SOCKET;
    a->config.agent_socket_path = config->agent_socket_path;
    a->config.task_handler = config->task_handler;
    a->config.handler_ctx = config->handler_ctx;
    a->state = AGENT_STATE_UNREGISTERED;
    a->running = false;
    pthread_mutex_init(&a->mutex, NULL);

    a->orch_client = ipc_client_init(a->config.orchestrator_socket);
    if (!a->orch_client) {
        free(a);
        return NULL;
    }

    log_info(MODULE, "agent initialised: %s v%s", a->config.name, a->config.version);
    return a;
}

int agent_register(agent_t *a) {
    if (!a) return -1;

    pthread_mutex_lock(&a->mutex);
    a->state = AGENT_STATE_REGISTERING;
    pthread_mutex_unlock(&a->mutex);

    if (ipc_client_connect(a->orch_client) < 0) {
        pthread_mutex_lock(&a->mutex);
        a->state = AGENT_STATE_ERROR;
        pthread_mutex_unlock(&a->mutex);
        log_error(MODULE, "failed to connect to orchestrator for registration");
        return -1;
    }

    agent_info_t info;
    memset(&info, 0, sizeof(info));
    strncpy(info.name, a->config.name, sizeof(info.name) - 1);
    strncpy(info.version, a->config.version, sizeof(info.version) - 1);
    if (a->config.agent_socket_path) {
        strncpy(info.socket_path, a->config.agent_socket_path,
                sizeof(info.socket_path) - 1);
    }
    info.cap_count = a->config.cap_count;
    for (int i = 0; i < a->config.cap_count && i < MAX_CAPABILITIES; i++) {
        strncpy(info.capabilities[i], a->config.capabilities[i],
                sizeof(info.capabilities[i]) - 1);
    }

    char reg_msg[4096];
    protocol_build_register(&info, reg_msg, sizeof(reg_msg));

    if (ipc_client_send(a->orch_client, reg_msg, strlen(reg_msg)) < 0) {
        pthread_mutex_lock(&a->mutex);
        a->state = AGENT_STATE_ERROR;
        pthread_mutex_unlock(&a->mutex);
        log_error(MODULE, "failed to send registration");
        return -1;
    }

    char resp_buf[4096];
    int n = ipc_client_recv_timeout(a->orch_client, resp_buf, sizeof(resp_buf), 3000);
    if (n <= 0) {
        pthread_mutex_lock(&a->mutex);
        a->state = AGENT_STATE_ERROR;
        pthread_mutex_unlock(&a->mutex);
        log_error(MODULE, "no response to registration");
        return -1;
    }

    if (strstr(resp_buf, "\"status\":\"success\"") ||
        strstr(resp_buf, "\"status\": \"success\"")) {
        pthread_mutex_lock(&a->mutex);
        a->state = AGENT_STATE_REGISTERED;
        pthread_mutex_unlock(&a->mutex);
        log_info(MODULE, "agent registered successfully: %s", a->config.name);
    } else {
        pthread_mutex_lock(&a->mutex);
        a->state = AGENT_STATE_ERROR;
        pthread_mutex_unlock(&a->mutex);
        log_error(MODULE, "registration failed: %s", resp_buf);
        return -1;
    }

    return 0;
}

int agent_start(agent_t *a) {
    if (!a) return -1;

    if (a->state != AGENT_STATE_REGISTERED) {
        if (agent_register(a) < 0) {
            return -1;
        }
    }

    if (a->config.agent_socket_path) {
        a->task_server = ipc_server_init(a->config.agent_socket_path, 64);
        if (!a->task_server) {
            log_error(MODULE, "failed to init task server");
            return -1;
        }
    }

    a->running = true;

    if (pthread_create(&a->heartbeat_thread, NULL, heartbeat_loop, a) != 0) {
        log_error(MODULE, "failed to start heartbeat thread");
        a->running = false;
        return -1;
    }

    pthread_mutex_lock(&a->mutex);
    a->state = AGENT_STATE_RUNNING;
    pthread_mutex_unlock(&a->mutex);

    log_info(MODULE, "agent started: %s", a->config.name);
    return 0;
}

int agent_run(agent_t *a) {
    if (!a) return -1;
    if (!a->task_server) return 0;

    log_info(MODULE, "agent entering main loop on %s",
             a->config.agent_socket_path);

    ipc_server_start(a->task_server, task_handler, a);
    return 0;
}

void agent_stop(agent_t *a) {
    if (!a) return;

    pthread_mutex_lock(&a->mutex);
    a->running = false;
    a->state = AGENT_STATE_SHUTDOWN;
    pthread_mutex_unlock(&a->mutex);

    if (a->task_server) {
        ipc_server_stop(a->task_server);
    }

    pthread_join(a->heartbeat_thread, NULL);
    log_info(MODULE, "agent stopped: %s", a->config.name);
}

void agent_cleanup(agent_t *a) {
    if (!a) return;

    agent_stop(a);

    if (a->task_server) {
        ipc_server_cleanup(a->task_server);
        a->task_server = NULL;
    }

    if (a->orch_client) {
        ipc_client_cleanup(a->orch_client);
        a->orch_client = NULL;
    }

    pthread_mutex_destroy(&a->mutex);
    free(a);
    log_info(MODULE, "agent cleaned up");
}

int agent_heartbeat(agent_t *a) {
    if (!a || !a->orch_client) return -1;

    char msg[256];
    protocol_build_heartbeat(a->config.name, msg, sizeof(msg));
    return ipc_client_send(a->orch_client, msg, strlen(msg));
}

agent_state_t agent_get_state(agent_t *a) {
    if (!a) return AGENT_STATE_ERROR;
    agent_state_t state;
    pthread_mutex_lock(&a->mutex);
    state = a->state;
    pthread_mutex_unlock(&a->mutex);
    return state;
}

const char *agent_get_name(agent_t *a) {
    return a ? a->config.name : NULL;
}

int agent_send_response(agent_t *a, int client_fd, const response_t *resp) {
    (void)a;
    if (!resp || client_fd < 0) return -1;

    char out[MAX_MESSAGE_SIZE];
    protocol_build_response(resp, out, sizeof(out));
    return ipc_server_send(client_fd, out, strlen(out));
}
