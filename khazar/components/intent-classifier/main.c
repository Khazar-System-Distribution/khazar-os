#include "include/common.h"
#include "logger/logger.h"
#include "ipc/ipc.h"
#include "config/config.h"
#include "protocol/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <errno.h>
#include <math.h>

#define MODULE "main"
#define MAX_KEYWORD_RULES 32

typedef struct {
    char keyword[64];
    char intent[MAX_INTENT_LEN];
    char target[MAX_TARGET_LEN];
} keyword_rule_t;

static volatile bool running = true;
static ipc_server_t *g_server = NULL;
static keyword_rule_t g_keywords[MAX_KEYWORD_RULES];
static int g_keyword_count = 0;

static void handle_signal(int sig) {
    (void)sig;
    log_info(MODULE, "signal received, shutting down...");
    running = false;
    if (g_server) ipc_server_stop(g_server);
}

static int connect_to_runtime(const char *socket_path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    return fd;
}

static int send_to_runtime(int fd, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int recv_from_runtime(int fd, char *buf, size_t buf_len) {
    memset(buf, 0, buf_len);
    size_t total = 0;
    while (total < buf_len - 1) {
        ssize_t n = recv(fd, buf + total, buf_len - total - 1, 0);
        if (n <= 0) break;
        total += (size_t)n;
        buf[total] = '\0';
        if (strchr(buf, '\n') && total > 2 && buf[total - 1] == '\n')
            break;
        if (total > 0 && buf[total - 1] == '}')
            break;
    }
    return (int)total;
}

static void parse_llm_response(const char *json, classifier_response_t *resp) {
    if (!json || !resp) return;

    const char *ip = strstr(json, "\"intent\"");
    if (ip) {
        ip = strchr(ip, ':');
        if (ip) {
            while (*ip && *ip != '"') ip++;
            if (*ip == '"') {
                ip++;
                int i = 0;
                while (*ip && *ip != '"' && i < (int)sizeof(resp->intent) - 1)
                    resp->intent[i++] = *ip++;
                resp->intent[i] = '\0';
            }
        }
    }

    const char *tp = strstr(json, "\"target\"");
    if (tp) {
        tp = strchr(tp, ':');
        if (tp) {
            while (*tp && *tp != '"') tp++;
            if (*tp == '"') {
                tp++;
                int i = 0;
                while (*tp && *tp != '"' && i < (int)sizeof(resp->target) - 1)
                    resp->target[i++] = *tp++;
                resp->target[i] = '\0';
            }
        }
    }

    const char *cp = strstr(json, "\"confidence\"");
    if (cp) {
        cp = strchr(cp, ':');
        if (cp) {
            while (*cp == ':' || *cp == ' ') cp++;
            resp->confidence = (float)atof(cp);
        }
    }
}

static void classify_with_runtime(const char *query, const char *runtime_sock,
                                   classifier_response_t *resp) {
    resp->fallback = false;

    int fd = connect_to_runtime(runtime_sock);
    if (fd < 0) {
        log_warn(MODULE, "cannot connect to model-runtime at %s: %s",
                 runtime_sock, strerror(errno));
        resp->fallback = true;
        return;
    }

    char prompt[MAX_PROMPT_LEN];
    snprintf(prompt, sizeof(prompt),
        "Classify this command: %s. Output JSON with intent, target, action.",
        query);

    char inference_req[MAX_PROMPT_LEN];
    snprintf(inference_req, sizeof(inference_req),
        "{\"type\":\"inference\",\"prompt\":\"%s\",\"max_tokens\":128}", prompt);

    if (send_to_runtime(fd, inference_req, strlen(inference_req)) < 0) {
        log_warn(MODULE, "failed to send to model-runtime");
        close(fd);
        resp->fallback = true;
        return;
    }

    char runtime_buf[MAX_RESPONSE_LEN];
    int n = recv_from_runtime(fd, runtime_buf, sizeof(runtime_buf));
    close(fd);

    if (n <= 0) {
        log_warn(MODULE, "no response from model-runtime");
        resp->fallback = true;
        return;
    }

    log_info(MODULE, "runtime response: %s", runtime_buf);

    const char *textp = strstr(runtime_buf, "\"text\"");
    if (textp) {
        textp = strchr(textp, ':');
        if (textp) {
            while (*textp && *textp != '"') textp++;
            if (*textp == '"') {
                textp++;
                int i = 0;
                while (*textp && *textp != '"' && i < (int)sizeof(resp->raw_output) - 1) {
                    if (*textp == '\\' && *(textp + 1)) {
                        char esc = *(textp + 1);
                        if (esc == 'n') resp->raw_output[i++] = '\n';
                        else if (esc == '"') resp->raw_output[i++] = '"';
                        else if (esc == '\\') resp->raw_output[i++] = '\\';
                        else resp->raw_output[i++] = *textp;
                        textp += 2;
                    } else {
                        resp->raw_output[i++] = *textp++;
                    }
                }
                resp->raw_output[i] = '\0';
            }
        }
    }

    if (resp->raw_output[0] != '\0') {
        parse_llm_response(resp->raw_output, resp);
    } else {
        parse_llm_response(runtime_buf, resp);
    }

    if (resp->confidence <= 0.0f) resp->confidence = 0.7f;
}

static void init_keyword_rules(void) {
    g_keyword_count = 0;

    keyword_rule_t rules[] = {
        {"open",           "open_application",    ""},
        {"launch",         "open_application",    ""},
        {"start",          "open_application",    ""},
        {"run",            "open_application",    ""},
        {"close",          "close_application",   ""},
        {"quit",           "close_application",   ""},
        {"exit",           "close_application",   ""},
        {"kill",           "close_application",   ""},
        {"shutdown",       "system_shutdown",     ""},
        {"power off",      "system_shutdown",     ""},
        {"reboot",         "system_reboot",       ""},
        {"restart",        "system_reboot",       ""},
        {"volume up",      "volume_increase",     ""},
        {"volume down",    "volume_decrease",     ""},
        {"mute",           "volume_mute",         ""},
        {"unmute",         "volume_unmute",       ""},
        {"brightness up",  "brightness_increase", ""},
        {"brightness down","brightness_decrease", ""},
        {"wifi on",        "wifi_enable",         ""},
        {"wifi off",       "wifi_disable",        ""},
        {"bluetooth on",   "bluetooth_enable",    ""},
        {"bluetooth off",  "bluetooth_disable",   ""},
        {"screenshot",     "screenshot",          ""},
        {"install",        "install_package",     ""},
        {"uninstall",      "remove_package",      ""},
        {"remove",         "remove_package",      ""},
        {"update",         "update_system",       ""},
        {"upgrade",        "update_system",       ""},
        {"network status", "network_status",      ""},
        {"battery",        "battery_status",      ""},
        {"lock",           "lock_screen",         ""},
        {"suspend",        "system_suspend",      ""},
    };

    int count = (int)(sizeof(rules) / sizeof(rules[0]));
    if (count > MAX_KEYWORD_RULES) count = MAX_KEYWORD_RULES;

    for (int i = 0; i < count; i++) {
        g_keywords[i] = rules[i];
    }
    g_keyword_count = count;
}

static int keyword_classify(const char *query, classifier_response_t *resp) {
    if (!query || !resp) return -1;

    char lower[MAX_QUERY_LEN];
    int i = 0;
    while (query[i] && i < (int)sizeof(lower) - 1) {
        lower[i] = (char)((unsigned char)query[i] >= 'A' && (unsigned char)query[i] <= 'Z'
                          ? query[i] + ('a' - 'A') : query[i]);
        i++;
    }
    lower[i] = '\0';

    int best_idx = -1;
    size_t best_len = 0;

    for (int k = 0; k < g_keyword_count; k++) {
        const char *kw = g_keywords[k].keyword;
        size_t kw_len = strlen(kw);
        if (strstr(lower, kw)) {
            if (kw_len > best_len) {
                best_len = kw_len;
                best_idx = k;
            }
        }
    }

    if (best_idx < 0) return -1;

    strncpy(resp->intent, g_keywords[best_idx].intent, sizeof(resp->intent) - 1);
    if (g_keywords[best_idx].target[0] != '\0') {
        strncpy(resp->target, g_keywords[best_idx].target, sizeof(resp->target) - 1);
    } else {
        const char *kw = g_keywords[best_idx].keyword;
        const char *pos = strstr(lower, kw);
        if (pos) {
            pos += strlen(kw);
            while (*pos == ' ') pos++;
            if (*pos != '\0') {
                strncpy(resp->target, pos, sizeof(resp->target) - 1);
            } else {
                strncpy(resp->target, "unknown", sizeof(resp->target) - 1);
            }
        }
    }

    resp->confidence = 0.5f;
    resp->fallback = true;
    return 0;
}

static void client_handler(int client_fd, const char *data, size_t len, void *ctx) {
    (void)ctx;
    char response[MAX_RESPONSE_LEN];
    classifier_message_type_t type;
    config_t *cfg = config_get();

    if (protocol_parse_type(data, len, &type) < 0) {
        protocol_build_error("invalid message", response, sizeof(response));
        ipc_server_send(client_fd, response, strlen(response));
        return;
    }

    switch (type) {
        case CMSG_PING:
            protocol_build_ping(response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case CMSG_VERSION:
            protocol_build_version(response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case CMSG_RELOAD:
            snprintf(response, sizeof(response),
                "{\"type\":\"classify_response\",\"status\":\"reloaded\"}");
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case CMSG_CLASSIFY: {
            classifier_request_t req;
            classifier_response_t resp;
            memset(&resp, 0, sizeof(resp));

            if (protocol_parse_classify(data, len, &req) != 0) {
                protocol_build_error("invalid classify request", response, sizeof(response));
                ipc_server_send(client_fd, response, strlen(response));
                break;
            }

            resp.id = req.id;
            resp.confidence = 0.0f;

            classify_with_runtime(req.query, cfg->model_runtime_socket, &resp);

            if (resp.fallback && cfg->fallback_to_local) {
                log_info(MODULE, "using fallback keyword matching for: %s", req.query);
                if (keyword_classify(req.query, &resp) == 0) {
                    log_info(MODULE, "fallback matched intent: %s", resp.intent);
                } else {
                    strncpy(resp.intent, "unknown", sizeof(resp.intent) - 1);
                    strncpy(resp.target, "unknown", sizeof(resp.target) - 1);
                    resp.confidence = 0.1f;
                    resp.fallback = true;
                }
            }

            protocol_build_classify_response(&resp, response, sizeof(response));
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
    log_info(MODULE, "AI Intent Classifier v0.1 starting...");

    config_load(config_path);
    config_t *cfg = config_get();
    logger_set_level((log_level_t)cfg->log_level);
    config_print(cfg);

    init_keyword_rules();
    log_info(MODULE, "loaded %d keyword rules for fallback", g_keyword_count);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    signal(SIGPIPE, SIG_IGN);

    g_server = ipc_server_init(cfg->socket_path, cfg->max_connections);
    if (!g_server) log_fatal(MODULE, "failed to init IPC server");

    log_info(MODULE, "AI Intent Classifier ready on %s", cfg->socket_path);
    log_info(MODULE, "model-runtime backend: %s", cfg->model_runtime_socket);
    log_info(MODULE, "fallback: %s", cfg->fallback_to_local ? "enabled" : "disabled");

    ipc_server_start(g_server, client_handler, NULL);

    log_info(MODULE, "shutting down...");
    ipc_server_cleanup(g_server);
    logger_cleanup();
    return 0;
}
