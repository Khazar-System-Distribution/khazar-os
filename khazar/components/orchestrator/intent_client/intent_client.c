#include "intent_client.h"
#include "logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <pthread.h>

#define MODULE "intent_client"
#define CLASSIFIER_SOCK_DEFAULT "/run/ai-intent-classifier.sock"
#define QUERY_TIMEOUT_SEC 5

struct intent_client {
    char socket_path[256];
    pthread_mutex_t mutex;
};

intent_client_t *intent_client_init(const char *socket_path) {
    intent_client_t *ic = calloc(1, sizeof(intent_client_t));
    if (!ic) return NULL;
    if (socket_path && socket_path[0])
        strncpy(ic->socket_path, socket_path, sizeof(ic->socket_path) - 1);
    else
        strncpy(ic->socket_path, CLASSIFIER_SOCK_DEFAULT, sizeof(ic->socket_path) - 1);
    pthread_mutex_init(&ic->mutex, NULL);
    log_info(MODULE, "initialized, target: %s", ic->socket_path);
    return ic;
}

static int connect_to(const char *path) {
    struct sockaddr_un addr;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    struct timeval tv = {.tv_sec = QUERY_TIMEOUT_SEC, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { close(fd); return -1; }
    return fd;
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
    int i = 0;
    while (*p && *p != '"' && i < (int)bufsz - 1) buf[i++] = *p++;
    buf[i] = '\0';
    return i;
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

int intent_client_classify(intent_client_t *ic, const char *query,
                            intent_t *intent, float *confidence) {
    if (!ic || !query || !intent) return -1;
    memset(intent, 0, sizeof(*intent));
    if (confidence) *confidence = 0.0f;

    pthread_mutex_lock(&ic->mutex);

    int fd = connect_to(ic->socket_path);
    if (fd < 0) {
        log_warn(MODULE, "classifier unreachable: %s", ic->socket_path);
        pthread_mutex_unlock(&ic->mutex);
        return -1;
    }

    char request[8192];
    snprintf(request, sizeof(request),
        "{\"type\":\"classify\",\"id\":1,\"query\":\"%s\"}", query);

    if (write(fd, request, strlen(request)) < 0) {
        log_error(MODULE, "classify send failed");
        close(fd);
        pthread_mutex_unlock(&ic->mutex);
        return -1;
    }

    char response[65536];
    ssize_t n = read(fd, response, sizeof(response) - 1);
    close(fd);

    if (n <= 0) {
        log_warn(MODULE, "no response from classifier");
        pthread_mutex_unlock(&ic->mutex);
        return -1;
    }

    response[n] = '\0';
    log_debug(MODULE, "classifier response: %.200s", response);

    extract_str(response, "intent", intent->required_capability, sizeof(intent->required_capability));
    extract_str(response, "target", intent->target, sizeof(intent->target));
    extract_str(response, "action", intent->action, sizeof(intent->action));

    float conf = 0.5f;
    extract_float(response, "confidence", &conf);
    if (confidence) *confidence = conf;

    log_info(MODULE, "Tier 1: '%s' -> intent=%s target=%s (%.2f)",
             query, intent->required_capability, intent->target, conf);

    pthread_mutex_unlock(&ic->mutex);
    return (intent->required_capability[0] && strcmp(intent->required_capability, "unknown") != 0) ? 0 : -1;
}

void intent_client_cleanup(intent_client_t *ic) {
    if (!ic) return;
    pthread_mutex_destroy(&ic->mutex);
    free(ic);
    log_info(MODULE, "cleaned up");
}
