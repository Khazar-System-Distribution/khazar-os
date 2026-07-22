#include "rule_client.h"
#include "logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <pthread.h>

#define MODULE "rule_client"
#define RULE_SOCK_DEFAULT "/run/ai-rule-engine.sock"
#define QUERY_TIMEOUT_SEC 5

struct rule_client {
    char        socket_path[256];
    pthread_mutex_t mutex;
};

rule_client_t *rule_client_init(const char *socket_path) {
    rule_client_t *rc = calloc(1, sizeof(rule_client_t));
    if (!rc) return NULL;

    if (socket_path && socket_path[0])
        strncpy(rc->socket_path, socket_path, sizeof(rc->socket_path) - 1);
    else
        strncpy(rc->socket_path, RULE_SOCK_DEFAULT, sizeof(rc->socket_path) - 1);

    pthread_mutex_init(&rc->mutex, NULL);
    log_info(MODULE, "initialized, target: %s", rc->socket_path);
    return rc;
}

static int connect_to_rule_engine(const char *path) {
    struct sockaddr_un addr;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    struct timeval tv;
    tv.tv_sec = QUERY_TIMEOUT_SEC;
    tv.tv_usec = 0;
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

static int extract_float(const char *json, const char *key, float *val) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return -1;

    p += strlen(search);
    *val = (float)atof(p);
    return 0;
}

int rule_client_query(rule_client_t *rc, const char *query, intent_t *intent, float *confidence) {
    if (!rc || !query || !intent) return -1;

    char request[4096], response[65536];
    memset(intent, 0, sizeof(*intent));
    if (confidence) *confidence = 0.0f;

    pthread_mutex_lock(&rc->mutex);

    int fd = connect_to_rule_engine(rc->socket_path);
    if (fd < 0) {
        log_warn(MODULE, "rule engine-e qosula bilmedi: %s", rc->socket_path);
        pthread_mutex_unlock(&rc->mutex);
        return -1;
    }

    snprintf(request, sizeof(request),
        "{\"id\":1,\"type\":\"request\",\"query\":\"%s\"}", query);

    ssize_t written = write(fd, request, strlen(request));
    if (written < 0) {
        log_error(MODULE, "query gonderile bilmedi");
        close(fd);
        pthread_mutex_unlock(&rc->mutex);
        return -1;
    }

    ssize_t n = read(fd, response, sizeof(response) - 1);
    close(fd);

    if (n <= 0) {
        log_warn(MODULE, "rule engine-den cavab alinmadi");
        pthread_mutex_unlock(&rc->mutex);
        return -1;
    }

    response[n] = '\0';
    log_debug(MODULE, "rule engine response: %s", response);

    if (strstr(response, "\"intent\":\"unknown\"")) {
        pthread_mutex_unlock(&rc->mutex);
        return -1;
    }

    char intent_str[128], target[256], action[128];
    float conf = 0.0f;

    if (extract_str(response, "intent", intent_str, sizeof(intent_str)) == 0) {
        strncpy(intent->required_capability, intent_str, sizeof(intent->required_capability) - 1);
    }
    if (extract_str(response, "target", target, sizeof(target)) == 0) {
        strncpy(intent->target, target, sizeof(intent->target) - 1);
    }
    if (extract_str(response, "action", action, sizeof(action)) == 0) {
        strncpy(intent->action, action, sizeof(intent->action) - 1);
    }
    if (extract_float(response, "confidence", &conf) == 0) {
        if (confidence) *confidence = conf;
    }

    log_info(MODULE, "query='%s' → intent=%s target=%s (%.2f)",
             query, intent_str, target, conf);

    pthread_mutex_unlock(&rc->mutex);
    return 0;
}

void rule_client_cleanup(rule_client_t *rc) {
    if (!rc) return;
    pthread_mutex_destroy(&rc->mutex);
    free(rc);
    log_info(MODULE, "cleaned up");
}
