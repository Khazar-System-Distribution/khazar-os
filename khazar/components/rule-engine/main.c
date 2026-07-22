#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "include/common.h"
#include "config/config.h"
#include "logger/logger.h"
#include "metrics/metrics.h"
#include "ipc/ipc.h"
#include "protocol/protocol.h"
#include "cache/cache.h"
#include "matcher/matcher.h"
#include "tokens/tokens.h"
#include "intent/intent.h"
#include "alias/alias.h"
#include "fuzzy/fuzzy.h"
#include "rules/rules.h"

static ipc_server_t *g_srv = NULL;
static volatile int g_running = 1;
static volatile int g_reload = 0;
static time_t g_uptime = 0;

static void signal_handler(int sig) {
    if (sig == SIGHUP) { g_reload = 1; return; }
    g_running = 0;
}

static void daemonize(void) {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);
    signal(SIGCHLD, SIG_IGN);
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) close(fd);
    }
}

static void write_pid_file(const char *path) {
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%d\n", getpid()); fclose(f); }
}

static int do_reload(void) {
    log_info("main", "SIGHUP: yeniden yukleme basladilir...");
    cache_clear();
    if (rules_load_all() > 0) {
        log_info("main", "qaydalar yeniden yuklendi");
    }
    g_reload = 0;
    return 0;
}

static void split_query(const char *query, char parts[MAX_MULTI_RESULTS][MAX_QUERY_LEN], int *count) {
    const char *separators[] = { " ve ", " & ", " həm ", " hem ", " + " };
    char temp[MAX_QUERY_LEN];
    char *trim_start, *trim_end;
    strncpy(temp, query, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    *count = 0;
    for (int i = 0; i < 5; i++) {
        char *pos = strstr(temp, separators[i]);
        if (pos) {
            *pos = '\0';
            trim_start = temp;
            while (*trim_start == ' ') trim_start++;
            strncpy(parts[0], trim_start, MAX_QUERY_LEN - 1);
            parts[0][MAX_QUERY_LEN - 1] = '\0';
            trim_end = parts[0] + strlen(parts[0]) - 1;
            while (trim_end > parts[0] && *trim_end == ' ') *(trim_end--) = '\0';

            trim_start = pos + strlen(separators[i]);
            while (*trim_start == ' ') trim_start++;
            strncpy(parts[1], trim_start, MAX_QUERY_LEN - 1);
            parts[1][MAX_QUERY_LEN - 1] = '\0';
            trim_end = parts[1] + strlen(parts[1]) - 1;
            while (trim_end > parts[1] && *trim_end == ' ') *(trim_end--) = '\0';
            if (parts[0][0] && parts[1][0]) *count = 2;
            return;
        }
    }
    strncpy(parts[0], query, MAX_QUERY_LEN - 1);
    parts[0][MAX_QUERY_LEN - 1] = '\0';
    *count = 1;
}

static bool confidence_ok(const match_result_t *r, config_t *cfg) {
    switch (r->source) {
        case MATCH_SOURCE_CACHE:  return r->confidence >= cfg->cache_confidence_threshold;
        case MATCH_SOURCE_REGEX:  return r->confidence >= cfg->regex_confidence_threshold;
        case MATCH_SOURCE_TOKEN:  return r->confidence >= cfg->token_confidence_threshold;
        case MATCH_SOURCE_INTENT: return r->confidence >= cfg->intent_confidence_threshold;
        case MATCH_SOURCE_ALIAS:  return r->confidence >= cfg->alias_confidence_threshold;
        case MATCH_SOURCE_FUZZY:  return true;
        default: return false;
    }
}

static int run_tier0_pipeline(const char *query, match_result_t *result) {
    config_t *cfg = config_get();
    struct timespec t1, t2;

    clock_gettime(CLOCK_MONOTONIC, &t1);
    memset(result, 0, sizeof(*result));
    result->intent = INTENT_UNKNOWN;

    if (cfg->enable_cache && cache_lookup(query, result) == 0 && confidence_ok(result, cfg))
        goto done;

    if (cfg->enable_intent_table && intent_lookup(query, result) == 0 && confidence_ok(result, cfg)) {
        if (cfg->enable_cache) cache_store(query, result);
        goto done;
    }

    if (cfg->enable_regex && matcher_match(query, result) == 0 && confidence_ok(result, cfg)) {
        if (cfg->enable_cache) cache_store(query, result);
        goto done;
    }

    if (cfg->enable_tokens && tokens_lookup(query, result) == 0 && confidence_ok(result, cfg)) {
        if (cfg->enable_cache) cache_store(query, result);
        goto done;
    }

    if (cfg->enable_alias && alias_resolve(query, result) == 0 && confidence_ok(result, cfg)) {
        if (cfg->enable_cache) cache_store(query, result);
        goto done;
    }

    if (cfg->enable_fuzzy && fuzzy_match(query, result) == 0) {
        if (cfg->enable_cache) cache_store(query, result);
        goto done;
    }

done:
    clock_gettime(CLOCK_MONOTONIC, &t2);
    double lat_us = (t2.tv_sec - t1.tv_sec) * 1e6 + (t2.tv_nsec - t1.tv_nsec) / 1e3;

    metrics_record_request();
    if (result->intent == INTENT_UNKNOWN) metrics_record_unknown();
    else metrics_record_hit(result->source);
    metrics_record_latency(lat_us);

    log_debug("pipeline", "'%s' -> %s (%.2f, %d, %.1fus)",
              query, intent_type_str(result->intent), result->confidence, result->source, lat_us);
    if (result->intent == INTENT_UNKNOWN) return -1;
    return 0;
}

static void handle_add_rule(const char *message, int client_fd) {
    char rule_type[64] = {0}, rule_data[MAX_MESSAGE_SIZE] = {0};
    char response[MAX_MESSAGE_SIZE];
    int added = 0;

    if (protocol_parse_add_rule(message, rule_type, sizeof(rule_type), rule_data, sizeof(rule_data)) != 0) {
        protocol_build_error(0, "INVALID_RULE_FORMAT", response, sizeof(response));
        ipc_server_send(client_fd, response, strlen(response));
        return;
    }

    log_info("ipc", "runtime qayda: type=%s data=%s", rule_type, rule_data);

    if (strcmp(rule_type, "regex") == 0) {
        char *patt = strtok(rule_data, "|");
        char *intent_s = strtok(NULL, "|");
        char *prio_s = strtok(NULL, "|");
        if (patt && intent_s) {
            int prio = prio_s ? atoi(prio_s) : 10;
            intent_type_t t = intent_type_from_str(intent_s);
            if (t != INTENT_UNKNOWN) {
                if (matcher_add_rule(patt, t, prio, RULE_SOURCE_RUNTIME) == 0) added = 1;
            }
        }
    } else if (strcmp(rule_type, "token") == 0) {
        char *tok = strtok(rule_data, "|");
        char *intent_s = strtok(NULL, "|");
        char *weight_s = strtok(NULL, "|");
        if (tok && intent_s) {
            int w = weight_s ? atoi(weight_s) : 5;
            intent_type_t t = intent_type_from_str(intent_s);
            if (t != INTENT_UNKNOWN) {
                if (tokens_add(tok, t, w, RULE_SOURCE_RUNTIME) == 0) added = 1;
            }
        }
    } else if (strcmp(rule_type, "intent") == 0) {
        char *query_s = strtok(rule_data, "|");
        char *intent_s = strtok(NULL, "|");
        char *target_s = strtok(NULL, "|");
        char *action_s = strtok(NULL, "|");
        if (query_s && intent_s) {
            intent_type_t t = intent_type_from_str(intent_s);
            if (t != INTENT_UNKNOWN) {
                if (intent_add(query_s, t, target_s ? target_s : "", action_s ? action_s : "", RULE_SOURCE_RUNTIME) == 0) added = 1;
            }
        }
    } else if (strcmp(rule_type, "alias") == 0) {
        char *als = strtok(rule_data, "|");
        char *canon = strtok(NULL, "|");
        if (als && canon) {
            if (alias_add(als, canon, RULE_SOURCE_RUNTIME) == 0) added = 1;
        }
    }

    if (added) {
        cache_clear();
        snprintf(response, sizeof(response), "{\"type\":\"response\",\"status\":\"rule_added\",\"rule_type\":\"%s\"}", rule_type);
    } else {
        snprintf(response, sizeof(response), "{\"type\":\"response\",\"status\":\"error\",\"message\":\"invalid_rule\"}");
    }
    ipc_server_send(client_fd, response, strlen(response));
}

static void handle_message(int client_fd, const char *message, size_t len, void *ctx) {
    (void)ctx;
    char response[MAX_MESSAGE_SIZE];
    parsed_request_t req;

    (void)len;
    message_type_t mtype = protocol_parse_type(message);

    switch (mtype) {
        case MSG_PING:
            protocol_build_ping_response(response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case MSG_VERSION: {
            int cs = cache_size();
            protocol_build_version_response(response, sizeof(response), g_uptime, cs);
            ipc_server_send(client_fd, response, strlen(response));
            break;
        }

        case MSG_METRICS: {
            pipeline_metrics_t m;
            metrics_get_snapshot(&m);
            protocol_build_metrics_response(response, sizeof(response), &m);
            ipc_server_send(client_fd, response, strlen(response));
            break;
        }

        case MSG_RELOAD:
            g_reload = 1;
            protocol_build_ping_response(response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            break;

        case MSG_ADD_RULE:
            handle_add_rule(message, client_fd);
            break;

        case MSG_REQUEST:
            if (protocol_parse_request(message, &req) == 0 && req.query[0] != '\0') {
                log_info("pipeline", "sorgu: '%s'", req.query);

                /* Check for multi-intent */
                char parts[MAX_MULTI_RESULTS][MAX_QUERY_LEN];
                int part_count = 0;
                split_query(req.query, parts, &part_count);

                if (part_count > 1) {
                    match_result_t multi_results[MAX_MULTI_RESULTS];
                    int found = 0;
                    for (int i = 0; i < part_count && i < MAX_MULTI_RESULTS; i++) {
                        if (run_tier0_pipeline(parts[i], &multi_results[i]) == 0) {
                            found++;
                        }
                    }
                    metrics_record_multi(found);
                    protocol_build_multi_response(multi_results, found, req.id, response, sizeof(response));
                } else {
                    match_result_t result;
                    run_tier0_pipeline(req.query, &result);
                    protocol_build_response(&result, req.id, response, sizeof(response));
                }
                ipc_server_send(client_fd, response, strlen(response));
            } else {
                protocol_build_error(req.id, "INVALID_REQUEST", response, sizeof(response));
                ipc_server_send(client_fd, response, strlen(response));
            }
            break;

        case MSG_REGISTER: {
            char name[256];
            if (protocol_parse_register(message, name, sizeof(name)) == 0) {
                log_info("ipc", "agent qeydiyyatdan kecdi: %s", name);
                snprintf(response, sizeof(response),
                    "{\"type\":\"response\",\"status\":\"registered\",\"name\":\"%s\"}", name);
                ipc_server_send(client_fd, response, strlen(response));
            }
            break;
        }

        default:
            protocol_build_error(0, "UNKNOWN_MESSAGE_TYPE", response, sizeof(response));
            ipc_server_send(client_fd, response, strlen(response));
            log_warn("ipc", "bilinmeyen mesaj tipi");
            break;
    }
}

int main(int argc, char *argv[]) {
    config_t *cfg;
    const char *config_path = NULL;

    if (argc > 1) config_path = argv[1];

    config_load(config_path);
    cfg = config_get();

    if (cfg->daemonize) daemonize();
    write_pid_file(cfg->pid_file);

    logger_init(cfg->log_file[0] ? cfg->log_file : NULL, (log_level_t)cfg->log_level);
    g_uptime = time(NULL);

    log_info("main", "=== AI Rule Engine v%d.%d.%d ===", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    log_info("main", "pid: %d, socket: %s", getpid(), cfg->socket_path);
    log_info("main", "modullar: cache=%s regex=%s tokens=%s intent=%s alias=%s fuzzy=%s daemon=%s",
             cfg->enable_cache ? "on" : "off", cfg->enable_regex ? "on" : "off",
             cfg->enable_tokens ? "on" : "off", cfg->enable_intent_table ? "on" : "off",
             cfg->enable_alias ? "on" : "off", cfg->enable_fuzzy ? "on" : "off",
             cfg->daemonize ? "on" : "off");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    /* Init modules */
    metrics_init();
    cache_init();
    matcher_init();
    tokens_init();
    intent_init();
    alias_init();
    fuzzy_init();
    rules_init(cfg->rules_dir);

    fuzzy_set_threshold(cfg->fuzzy_threshold);
    metrics_set_cache_size((uint64_t)cache_size());

    /* Load rules */
    matcher_load_defaults();
    tokens_load_defaults();
    intent_load_defaults();
    alias_load_defaults();
    rules_load_all();

    log_info("main", "Tier 0 pipeline hazirdir (%d intent, %d regex, %d token, %d alias)",
             MAX_INTENT_ENTRIES, MAX_REGEX_RULES, MAX_TOKENS, MAX_ALIASES);

    /* IPC */
    g_srv = ipc_server_init(cfg->socket_path, cfg->max_connections);
    if (!g_srv) log_fatal("main", "IPC server yaradila bilmedi");
    if (ipc_server_start_async(g_srv, handle_message, NULL) != 0) log_fatal("main", "IPC server basladila bilmedi");

    log_info("main", "Rule Engine isleyir (%s)",
             cfg->daemonize ? "daemon" : "Ctrl+C dayandirar");

    time_t last_cleanup = time(NULL);

    while (g_running) {
        sleep(1);

        if (g_reload) do_reload();

        time_t now = time(NULL);
        if (now - last_cleanup >= cfg->cache_cleanup_interval) {
            cache_clear();
            metrics_set_cache_size(0);
            last_cleanup = now;
        }

        metrics_set_cache_size((uint64_t)cache_size());
    }

    log_info("main", "dayandirilir...");
    ipc_server_cleanup(g_srv);
    rules_cleanup();
    cache_cleanup();
    matcher_cleanup();
    tokens_cleanup();
    intent_cleanup();
    alias_cleanup();
    fuzzy_cleanup();
    metrics_cleanup();
    unlink(cfg->pid_file);
    logger_cleanup();

    return 0;
}
