#include "include/common.h"
#include "logger/logger.h"
#include "ipc/ipc.h"
#include "config/config.h"
#include "inference/inference.h"
#include "protocol/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MODULE "main"

static volatile bool running = true;
static ipc_server_t       *g_server = NULL;
static inference_engine_t *g_engine = NULL;

static void handle_signal(int sig) {
    (void)sig;
    running = false;
    if (g_server) ipc_server_stop(g_server);
}

static void client_handler(int client_fd, const char *data, size_t len, void *ctx) {
    (void)ctx;
    char response[MAX_MESSAGE_SIZE];
    runtime_message_type_t type;

    if (protocol_parse_type(data, len, &type) < 0) {
        protocol_build_error("invalid message", response, sizeof(response));
        ipc_server_send(client_fd, response, strlen(response));
        return;
    }

    switch (type) {
        case RMSG_PING:
            protocol_build_ping(response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case RMSG_VERSION: {
            model_info_t info = {
                .backend = INFERENCE_BACKEND_MOCK,
                .state = inference_get_state(g_engine),
                .context_size = 2048,
            };
            strncpy(info.model_name, inference_get_model_name(g_engine) ? inference_get_model_name(g_engine) : "none", sizeof(info.model_name) - 1);
            protocol_build_version(&info, response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;
        }

        case RMSG_STATUS: {
            model_info_t info = {
                .backend = INFERENCE_BACKEND_MOCK,
                .state = inference_get_state(g_engine),
                .loaded_at = time(NULL),
            };
            strncpy(info.model_name, inference_get_model_name(g_engine) ? inference_get_model_name(g_engine) : "none", sizeof(info.model_name) - 1);
            protocol_build_status(&info, response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;
        }

        case RMSG_LOAD_MODEL:
            if (inference_load_model(g_engine) == 0) {
                snprintf(response, sizeof(response),
                    "{\"type\":\"inference_response\",\"status\":\"model_loaded\",\"model\":\"%s\"}",
                    inference_get_model_name(g_engine));
            } else {
                protocol_build_error("failed to load model", response, sizeof(response));
            }
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case RMSG_UNLOAD_MODEL:
            inference_unload_model(g_engine);
            snprintf(response, sizeof(response), "{\"type\":\"inference_response\",\"status\":\"model_unloaded\"}");
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case RMSG_INFERENCE: {
            inference_request_t req;
            inference_response_t resp;

            if (protocol_parse_inference(data, len, &req) == 0) {
                log_info(MODULE, "inference: prompt='%s...' max_tokens=%d",
                         req.prompt, req.max_tokens);
                inference_generate(g_engine, &req, &resp);
                protocol_build_inference_response(&resp, response, sizeof(response));
            } else {
                protocol_build_error("invalid inference request", response, sizeof(response));
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
    log_info(MODULE, "AI Model Runtime v0.1 starting...");

    config_load(config_path);
    config_t *cfg = config_get();
    logger_set_level((log_level_t)cfg->log_level);
    config_print(cfg);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    g_engine = inference_init(cfg->model_path, cfg->backend,
                               cfg->context_size, cfg->num_threads);
    if (!g_engine) log_fatal(MODULE, "failed to init inference engine");

    if (inference_load_model(g_engine) == 0) {
        log_info(MODULE, "model loaded: %s", inference_get_model_name(g_engine));
    } else {
        log_warn(MODULE, "model loading deferred");
    }

    g_server = ipc_server_init(cfg->socket_path, cfg->max_connections);
    if (!g_server) log_fatal(MODULE, "failed to init IPC server");

    log_info(MODULE, "AI Model Runtime ready on %s", cfg->socket_path);
    log_info(MODULE, "backend: %s | model: %s", cfg->backend, inference_get_model_name(g_engine));

    ipc_server_start(g_server, client_handler, NULL);

    log_info(MODULE, "shutting down...");
    ipc_server_cleanup(g_server);
    inference_cleanup(g_engine);
    logger_cleanup();
    return 0;
}
