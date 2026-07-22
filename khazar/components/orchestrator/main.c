#include "include/common.h"
#include "logger/logger.h"
#include "config/config.h"
#include "protocol/protocol.h"
#include "ipc/ipc.h"
#include "registry/registry.h"
#include "router/router.h"
#include "session/session.h"
#include "metrics/metrics.h"
#include "rule_client/rule_client.h"
#include "policy_client/policy_client.h"
#include "agent_client/agent_client.h"
#include "intent_client/intent_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MODULE "main"

static volatile bool running = true;

static config_t          *g_config = NULL;
static ipc_server_t      *g_server = NULL;
static registry_t        *g_registry = NULL;
static router_t          *g_router = NULL;
static session_manager_t *g_session = NULL;
static metrics_t         *g_metrics = NULL;
static rule_client_t     *g_rule_client = NULL;
static policy_client_t   *g_policy_client = NULL;
static agent_client_t    *g_agent_client = NULL;
static intent_client_t   *g_intent_client = NULL;

static void handle_signal(int sig) {
    (void)sig;
    log_info(MODULE, "signal received, shutting down...");
    running = false;
    if (g_server) ipc_server_stop(g_server);
}

static void send_response(int fd, response_t *resp, char *buf, size_t bufsz) {
    protocol_build_response(resp, buf, bufsz);
    ipc_server_send(fd, buf, strlen(buf));
}

static void on_request_handler(int client_fd, const char *data, size_t len, void *ctx) {
    (void)ctx;

    message_type_t type;
    request_t req;
    response_t resp;
    char out_buf[MAX_MESSAGE_SIZE];

    memset(&req, 0, sizeof(req));
    memset(&resp, 0, sizeof(resp));

    if (protocol_parse(data, len, &type, &req) < 0) {
        resp.id = 0;
        resp.status = STATUS_ERROR;
        resp.error_code = ERR_INVALID_REQUEST;
        snprintf(resp.payload, sizeof(resp.payload), "invalid message format");
        send_response(client_fd, &resp, out_buf, sizeof(out_buf));
        return;
    }

    switch (type) {
        case MSG_PING:
            ipc_server_send(client_fd, "{\"status\":\"alive\"}", 17);
            break;

        case MSG_REQUEST: {
            resp.id = req.id;

            log_info(MODULE, "request: %s", req.query);

            /* 1. Intent resolution: Rule Engine → local router → classifier */
            intent_t intent;
            memset(&intent, 0, sizeof(intent));
            int resolved = 0;
            float confidence = 0.0f;

            /* Tier 0: Rule Engine */
            if (g_rule_client) {
                if (rule_client_query(g_rule_client, req.query, &intent, &confidence) == 0) {
                    log_info(MODULE, "Tier 0: %s (%.2f)", intent.required_capability, confidence);
                    resolved = 1;
                }
            }

            /* Tier 0 fallback: local keyword router */
            if (!resolved && g_router && router_resolve(g_router, &req, &intent) == 0) {
                log_info(MODULE, "local router: %s", intent.required_capability);
                resolved = 1;
            }

            /* Tier 1: LLM classifier (for UNKNOWN queries) */
            if (!resolved && g_intent_client) {
                float tier1_conf = 0.0f;
                if (intent_client_classify(g_intent_client, req.query, &intent, &tier1_conf) == 0) {
                    log_info(MODULE, "Tier 1: %s (%.2f)", intent.required_capability, tier1_conf);
                    resolved = 1;
                    confidence = tier1_conf;
                }
            }

            if (!resolved) {
                resp.status = STATUS_SUCCESS;
                snprintf(resp.payload, sizeof(resp.payload), "no intent matched for query");
                send_response(client_fd, &resp, out_buf, sizeof(out_buf));
                break;
            }

            /* 2. Find agent by capability */
            agent_info_t *agent = registry_find_by_capability(g_registry, intent.required_capability);
            if (!agent) {
                resp.status = STATUS_ERROR;
                resp.error_code = ERR_AGENT_UNAVAILABLE;
                snprintf(resp.payload, sizeof(resp.payload),
                         "no agent for capability '%s'", intent.required_capability);
                log_warn(MODULE, "%s", resp.payload);
                send_response(client_fd, &resp, out_buf, sizeof(out_buf));
                break;
            }

            log_info(MODULE, "agent found: %s (%s)", agent->name, agent->socket_path);

            /* 3. Policy check */
            bool allowed = false;
            char policy_reason[256] = {0};
            if (g_policy_client) {
                if (policy_client_check(g_policy_client, agent->name,
                                        intent.required_capability, intent.target,
                                        &allowed, policy_reason, sizeof(policy_reason)) == 0) {
                    if (!allowed) {
                        resp.status = STATUS_ERROR;
                        resp.error_code = ERR_POLICY_DENIED;
                        snprintf(resp.payload, sizeof(resp.payload), "policy denied: %s",
                                 policy_reason[0] ? policy_reason : "action not permitted");
                        log_warn(MODULE, "POLICY DENIED: agent=%s cap=%s reason=%s",
                                 agent->name, intent.required_capability, policy_reason);
                        send_response(client_fd, &resp, out_buf, sizeof(out_buf));
                        break;
                    }
                    log_info(MODULE, "POLICY ALLOW: agent=%s cap=%s", agent->name, intent.required_capability);
                } else {
                    log_warn(MODULE, "policy engine unreachable, proceeding anyway");
                }
            }

            /* 4. Dispatch to agent */
            if (g_agent_client) {
                agent_client_dispatch(g_agent_client, agent, &req, &resp);
            } else {
                resp.status = STATUS_SUCCESS;
                snprintf(resp.payload, sizeof(resp.payload),
                         "routed to %s (no agent client)", agent->name);
            }

            if (g_metrics) {
                metrics_record_request(g_metrics, 0, resolved);
            }

            send_response(client_fd, &resp, out_buf, sizeof(out_buf));
            break;
        }

        case MSG_REGISTER: {
            agent_info_t agent;
            memset(&agent, 0, sizeof(agent));

            const char *name_p = strstr(data, "\"name\"");
            if (name_p) {
                name_p = strchr(name_p, ':');
                if (name_p) {
                    while (*name_p && *name_p != '"') name_p++;
                    if (*name_p == '"') {
                        name_p++;
                        int i = 0;
                        while (*name_p && *name_p != '"' && i < MAX_NAME_LEN - 1)
                            agent.name[i++] = *name_p++;
                    }
                }
            }

            const char *ver_p = strstr(data, "\"version\"");
            if (ver_p) {
                ver_p = strchr(ver_p, ':');
                if (ver_p) {
                    while (*ver_p && *ver_p != '"') ver_p++;
                    if (*ver_p == '"') {
                        ver_p++;
                        int i = 0;
                        while (*ver_p && *ver_p != '"' && i < 15)
                            agent.version[i++] = *ver_p++;
                    }
                }
            }

            const char *sock_p = strstr(data, "\"socket\"");
            if (!sock_p) sock_p = strstr(data, "\"socket_path\"");
            if (sock_p) {
                sock_p = strchr(sock_p, ':');
                if (sock_p) {
                    while (*sock_p && *sock_p != '"') sock_p++;
                    if (*sock_p == '"') {
                        sock_p++;
                        int i = 0;
                        while (*sock_p && *sock_p != '"' && i < MAX_SOCKET_LEN - 1)
                            agent.socket_path[i++] = *sock_p++;
                    }
                }
            }

            const char *caps_p = strstr(data, "\"capabilities\"");
            if (caps_p) {
                caps_p = strchr(caps_p, '[');
                if (caps_p) {
                    caps_p++;
                    while (*caps_p && *caps_p != ']' && agent.cap_count < MAX_CAPABILITIES) {
                        while (*caps_p && *caps_p != '"') caps_p++;
                        if (*caps_p == '"') {
                            caps_p++;
                            int i = 0;
                            while (*caps_p && *caps_p != '"' && i < MAX_NAME_LEN - 1)
                                agent.capabilities[agent.cap_count][i++] = *caps_p++;
                            agent.cap_count++;
                            if (*caps_p) caps_p++;
                        }
                        while (*caps_p && *caps_p != ',' && *caps_p != ']') caps_p++;
                        if (*caps_p == ',') caps_p++;
                    }
                }
            }

            agent.alive = true;
            agent.last_heartbeat = time(NULL);

            if (agent.name[0]) {
                registry_register(g_registry, &agent);
                resp.id = 0;
                resp.status = STATUS_SUCCESS;
                snprintf(resp.payload, sizeof(resp.payload),
                         "agent %s registered (cap: %d, sock: %s)",
                         agent.name, agent.cap_count, agent.socket_path);
                log_info(MODULE, "agent registered: %s v%s sock=%s caps=%d",
                         agent.name, agent.version, agent.socket_path, agent.cap_count);
            } else {
                resp.id = 0;
                resp.status = STATUS_ERROR;
                resp.error_code = ERR_INVALID_REQUEST;
                snprintf(resp.payload, sizeof(resp.payload), "invalid registration");
            }

            send_response(client_fd, &resp, out_buf, sizeof(out_buf));
            break;
        }

        default:
            resp.id = 0;
            resp.status = STATUS_ERROR;
            resp.error_code = ERR_INVALID_REQUEST;
            snprintf(resp.payload, sizeof(resp.payload), "unknown message type");
            send_response(client_fd, &resp, out_buf, sizeof(out_buf));
            break;
    }
}

static void cleanup(void) {
    ipc_server_cleanup(g_server);
    agent_client_cleanup(g_agent_client);
    intent_client_cleanup(g_intent_client);
    policy_client_cleanup(g_policy_client);
    rule_client_cleanup(g_rule_client);
    session_cleanup(g_session);
    router_cleanup(g_router);
    registry_cleanup(g_registry);
    metrics_cleanup(g_metrics);
    logger_cleanup();
}

int main(int argc, char *argv[]) {
    const char *config_path = NULL;
    if (argc > 1) config_path = argv[1];

    logger_init(NULL, LOG_INFO);
    log_info(MODULE, "AI Orchestrator v0.2 starting...");

    config_t cfg;
    config_load(config_path, &cfg);
    g_config = &cfg;
    logger_set_level(cfg.log_level);
    config_print(g_config);

    g_metrics = metrics_init(cfg.enable_metrics);
    g_registry = registry_init();
    g_router = router_init(g_registry);
    g_session = session_init(MAX_SESSIONS, SESSION_TIMEOUT);
    g_rule_client = rule_client_init(cfg.rule_engine_socket);
    g_policy_client = policy_client_init(cfg.policy_engine_socket);
    g_agent_client = agent_client_init();
    g_intent_client = intent_client_init(NULL);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    g_server = ipc_server_init(cfg.socket_path, cfg.max_connections);
    if (!g_server) {
        log_fatal(MODULE, "failed to initialize IPC server");
    }

    log_info(MODULE, "AI Orchestrator v0.2 ready");
    log_info(MODULE, "pipeline: request → rule-engine → intent-classifier → policy-engine → agent");
    log_info(MODULE, "listening on %s", cfg.socket_path);

    ipc_server_start(g_server, on_request_handler, NULL);

    log_info(MODULE, "shutting down...");
    cleanup();
    return 0;
}
