#include "policy_client.h"
#include "logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <pthread.h>

#define MODULE "policy_client"
#define POLICY_SOCK_DEFAULT "/run/ai-policy-engine.sock"
#define QUERY_TIMEOUT_SEC 3

struct policy_client {
    char socket_path[256];
    pthread_mutex_t mutex;
};

policy_client_t *policy_client_init(const char *socket_path) {
    policy_client_t *pc = calloc(1, sizeof(policy_client_t));
    if (!pc) return NULL;

    if (socket_path && socket_path[0])
        strncpy(pc->socket_path, socket_path, sizeof(pc->socket_path) - 1);
    else
        strncpy(pc->socket_path, POLICY_SOCK_DEFAULT, sizeof(pc->socket_path) - 1);

    pthread_mutex_init(&pc->mutex, NULL);
    log_info(MODULE, "initialized, target: %s", pc->socket_path);
    return pc;
}

static int connect_to_policy_engine(const char *path) {
    struct sockaddr_un addr;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    struct timeval tv = {.tv_sec = QUERY_TIMEOUT_SEC, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static int extract_str(const char *json, const char *key, char *buf, size_t bufsz) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char *p = strstr(json, search);
    if (!p) return -1;

    p += strlen(search);
    const char *end = strchr(p, '"');
    if (!end) { strncpy(buf, p, bufsz - 1); return 0; }

    size_t len = (size_t)(end - p);
    if (len >= bufsz) len = bufsz - 1;
    memcpy(buf, p, len);
    buf[len] = '\0';
    return 0;
}

int policy_client_check(policy_client_t *pc, const char *agent_name,
                        const char *capability, const char *target,
                        bool *allowed, char *reason, size_t reason_len) {
    if (!pc || !agent_name || !capability || !allowed) return -1;

    *allowed = false;
    if (reason) reason[0] = '\0';

    pthread_mutex_lock(&pc->mutex);

    int fd = connect_to_policy_engine(pc->socket_path);
    if (fd < 0) {
        log_warn(MODULE, "policy engine-e qosula bilmedi: %s", pc->socket_path);
        pthread_mutex_unlock(&pc->mutex);
        return -1;
    }

    char request[4096];
    snprintf(request, sizeof(request),
        "{\"type\":\"policy_check\",\"id\":1,\"agent\":\"%s\",\"capability\":\"%s\",\"target\":\"%s\"}",
        agent_name, capability, target ? target : "");

    ssize_t written = write(fd, request, strlen(request));
    if (written < 0) {
        log_error(MODULE, "policy check gonderile bilmedi");
        close(fd);
        pthread_mutex_unlock(&pc->mutex);
        return -1;
    }

    char response[4096];
    ssize_t n = read(fd, response, sizeof(response) - 1);
    close(fd);

    if (n <= 0) {
        log_warn(MODULE, "policy engine-den cavab alinmadi");
        pthread_mutex_unlock(&pc->mutex);
        return -1;
    }

    response[n] = '\0';
    log_debug(MODULE, "policy engine response: %s", response);

    char action[32] = {0};
    extract_str(response, "action", action, sizeof(action));

    if (strcmp(action, "allow") == 0) {
        *allowed = true;
    }

    if (reason) {
        extract_str(response, "reason", reason, reason_len);
    }

    log_info(MODULE, "agent=%s cap=%s → %s", agent_name, capability,
             *allowed ? "ALLOW" : "DENY");

    pthread_mutex_unlock(&pc->mutex);
    return 0;
}

void policy_client_cleanup(policy_client_t *pc) {
    if (!pc) return;
    pthread_mutex_destroy(&pc->mutex);
    free(pc);
    log_info(MODULE, "cleaned up");
}
