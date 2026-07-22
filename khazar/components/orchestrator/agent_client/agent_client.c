#include "agent_client.h"
#include "logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#define MODULE "agent_client"
#define DISPATCH_TIMEOUT_SEC 3

struct agent_client {
    int dummy;
};

agent_client_t *agent_client_init(void) {
    agent_client_t *ac = calloc(1, sizeof(agent_client_t));
    if (!ac) return NULL;
    log_info(MODULE, "agent client initialized");
    return ac;
}

static int extract_str(const char *json, const char *key, char *buf, size_t bufsz) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return -1;

    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return -1;
    p++;

    const char *end = strchr(p, '"');
    if (!end) { strncpy(buf, p, bufsz - 1); return 0; }

    size_t len = (size_t)(end - p);
    if (len >= bufsz) len = bufsz - 1;
    memcpy(buf, p, len);
    buf[len] = '\0';
    return 0;
}

int agent_client_dispatch(agent_client_t *ac, const agent_info_t *agent,
                           const request_t *req, response_t *resp) {
    (void)ac;
    if (!agent || !req || !resp) return -1;

    resp->id = req->id;
    resp->status = STATUS_ERROR;

    if (!agent->socket_path[0]) {
        snprintf(resp->payload, sizeof(resp->payload),
                 "agent %s has no socket path", agent->name);
        resp->error_code = ERR_AGENT_UNAVAILABLE;
        log_warn(MODULE, "%s", resp->payload);
        return -1;
    }

    struct sockaddr_un addr;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        snprintf(resp->payload, sizeof(resp->payload), "socket creation failed");
        resp->error_code = ERR_SYSTEM;
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, agent->socket_path, sizeof(addr.sun_path) - 1);

    struct timeval tv = {.tv_sec = DISPATCH_TIMEOUT_SEC, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        snprintf(resp->payload, sizeof(resp->payload),
                 "cannot connect to agent %s at %s", agent->name, agent->socket_path);
        resp->error_code = ERR_AGENT_UNAVAILABLE;
        log_warn(MODULE, "%s", resp->payload);
        close(fd);
        return -1;
    }

    char request[65536];
    snprintf(request, sizeof(request),
        "{\"id\":%llu,\"type\":\"request\",\"query\":\"%s\",\"capability\":\"%s\"}",
        (unsigned long long)req->id, req->query,
        agent->capabilities[0]);

    ssize_t written = write(fd, request, strlen(request));
    if (written < 0) {
        snprintf(resp->payload, sizeof(resp->payload),
                 "failed to send task to %s", agent->name);
        resp->error_code = ERR_AGENT_DISCONNECTED;
        log_error(MODULE, "%s", resp->payload);
        close(fd);
        return -1;
    }

    char response_buf[65536];
    ssize_t n = read(fd, response_buf, sizeof(response_buf) - 1);
    close(fd);

    if (n <= 0) {
        snprintf(resp->payload, sizeof(resp->payload),
                 "agent %s did not respond", agent->name);
        resp->error_code = ERR_AGENT_TIMEOUT;
        log_warn(MODULE, "%s", resp->payload);
        return -1;
    }

    response_buf[n] = '\0';
    log_debug(MODULE, "agent response: %s", response_buf);

    char status_str[32] = {0}, message[65536] = {0};
    extract_str(response_buf, "status", status_str, sizeof(status_str));
    extract_str(response_buf, "message", message, sizeof(message));

    if (strstr(response_buf, "\"status\":\"success\"") || strcmp(status_str, "success") == 0) {
        resp->status = STATUS_SUCCESS;
        resp->error_code = ERR_NONE;
        snprintf(resp->payload, sizeof(resp->payload), "%s", message[0] ? message : "task executed successfully");
    } else {
        resp->status = STATUS_ERROR;
        resp->error_code = ERR_INTERNAL;
        snprintf(resp->payload, sizeof(resp->payload), "%s", message[0] ? message : "agent returned error");
    }

    log_info(MODULE, "dispatched to %s: %s", agent->name,
             resp->status == STATUS_SUCCESS ? "SUCCESS" : "FAILED");

    return 0;
}

void agent_client_cleanup(agent_client_t *ac) {
    if (!ac) return;
    free(ac);
    log_info(MODULE, "agent client cleaned up");
}
