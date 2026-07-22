#include <ai-sdk/common.h>
#include <ai-sdk/agent.h>
#include <ai-sdk/logger.h>
#include <ai-sdk/ipc.h>
#include <ai-sdk/protocol.h>
#include <ai-sdk/events.h>
#include <ai-sdk/errors.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MODULE "basic-agent"

static volatile bool running = true;

static void signal_handler(int sig) {
    (void)sig;
    running = false;
}

static int handle_task(const request_t *req, response_t *resp, void *ctx) {
    (void)ctx;

    log_info(MODULE, "task received: query='%s' id=%llu",
             req->query, (unsigned long long)req->id);

    resp->id = req->id;
    resp->status = STATUS_SUCCESS;
    snprintf(resp->payload, sizeof(resp->payload),
             "processed: %s", req->query);

    return 0;
}

int main(void) {
    logger_init(LOG_INFO);
    log_info(MODULE, "Basic Agent starting...");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    const char *caps[] = {"open_application", "close_application"};
    agent_config_t config = {
        .name = "basic-agent",
        .version = "0.1.0",
        .capabilities = {caps[0], caps[1]},
        .cap_count = 2,
        .orchestrator_socket = DEFAULT_ORCHESTRATOR_SOCKET,
        .agent_socket_path = "/run/ai-basic-agent.sock",
        .task_handler = handle_task,
        .handler_ctx = NULL,
    };

    agent_t *agent = agent_init(&config);
    if (!agent) {
        log_fatal(MODULE, "failed to initialise agent");
    }

    if (agent_register(agent) < 0) {
        log_warn(MODULE, "registration failed - orchestrator may not be running");
    } else {
        log_info(MODULE, "registered with orchestrator");
    }

    if (agent_start(agent) < 0) {
        log_fatal(MODULE, "failed to start agent");
    }

    log_info(MODULE, "agent is running. Press Ctrl+C to stop.");
    log_info(MODULE, "listening on %s", config.agent_socket_path);

    while (running && agent_get_state(agent) == AGENT_STATE_RUNNING) {
        sleep(1);
    }

    log_info(MODULE, "shutting down...");
    agent_cleanup(agent);
    logger_cleanup();

    return 0;
}
