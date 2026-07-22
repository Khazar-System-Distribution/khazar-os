#include "include/common.h"
#include "logger/logger.h"
#include "ipc/ipc.h"
#include "config/config.h"
#include "policy/policy.h"
#include "protocol/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MODULE "main"

static volatile bool running = true;
static ipc_server_t   *g_server = NULL;
static policy_store_t *g_policy = NULL;

static void handle_signal(int sig) {
    (void)sig;
    log_info(MODULE, "signal received, shutting down...");
    running = false;
    if (g_server) ipc_server_stop(g_server);
}

static void client_handler(int client_fd, const char *data, size_t len, void *ctx) {
    (void)ctx;
    char response[1024];
    policy_message_type_t type;

    if (protocol_parse(data, len, &type) < 0) {
        protocol_build_error("invalid message", response, sizeof(response));
        ipc_server_send(client_fd, response, strlen(response));
        return;
    }

    switch (type) {
        case PMSG_PING:
            protocol_build_ping(response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case PMSG_VERSION:
            protocol_build_version(response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case PMSG_RELOAD: {
            config_t *cfg = config_get();
            policy_reload(g_policy, cfg->policy_dir);
            snprintf(response, sizeof(response),
                "{\"type\":\"policy_response\",\"status\":\"reloaded\"}");
            ipc_server_send(client_fd, response, strlen(response));
            break;
        }

        case PMSG_ADD_RULE: {
            policy_rule_t rule;
            if (protocol_parse_add_rule(data, len, &rule) == 0) {
                policy_add_rule(g_policy, &rule);
                protocol_build_rule_added(rule.capability[0] ? rule.capability : "unnamed",
                                         response, sizeof(response));
            } else {
                protocol_build_error("invalid rule format", response, sizeof(response));
            }
            ipc_server_send(client_fd, response, strlen(response));
            break;
        }

        case PMSG_POLICY_CHECK: {
            policy_request_t req;
            policy_response_t resp;

            if (protocol_parse_policy_check(data, len, &req) == 0) {
                policy_check(g_policy, &req, &resp);
                protocol_build_response(&resp, response, sizeof(response));
            } else {
                protocol_build_error("invalid policy check request", response, sizeof(response));
            }
            ipc_server_send(client_fd, response, strlen(response));
            break;
        }

        default:
            protocol_build_error("unknown message type", response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;
    }
}

int main(int argc, char *argv[]) {
    const char *config_path = NULL;
    if (argc > 1) config_path = argv[1];

    logger_init(NULL, LOG_INFO);
    log_info(MODULE, "AI Policy Engine v0.1 starting...");

    config_load(config_path);
    config_t *cfg = config_get();
    logger_set_level((log_level_t)cfg->log_level);
    config_print(cfg);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    g_policy = policy_init();
    if (!g_policy) {
        log_fatal(MODULE, "failed to initialise policy store");
    }

    int rules_loaded = policy_load_dir(g_policy, cfg->policy_dir);
    log_info(MODULE, "loaded %d policy rules", rules_loaded);

    g_server = ipc_server_init(cfg->socket_path, cfg->max_connections);
    if (!g_server) {
        log_fatal(MODULE, "failed to initialise IPC server");
    }

    log_info(MODULE, "AI Policy Engine v0.1 ready");
    log_info(MODULE, "listening on %s", cfg->socket_path);

    ipc_server_start(g_server, client_handler, NULL);

    log_info(MODULE, "shutting down...");
    ipc_server_cleanup(g_server);
    policy_cleanup(g_policy);
    logger_cleanup();

    return 0;
}
