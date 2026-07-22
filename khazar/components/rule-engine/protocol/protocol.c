#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol/protocol.h"

message_type_t protocol_parse_type(const char *json) {
    if (!json) return MSG_UNKNOWN;
    if (strstr(json, "\"type\":\"ping\"") || strstr(json, "\"type\": \"ping\""))       return MSG_PING;
    if (strstr(json, "\"type\":\"request\"") || strstr(json, "\"type\": \"request\"")) return MSG_REQUEST;
    if (strstr(json, "\"type\":\"register\"") || strstr(json, "\"type\": \"register\"")) return MSG_REGISTER;
    if (strstr(json, "\"type\":\"version\"") || strstr(json, "\"type\": \"version\"")) return MSG_VERSION;
    if (strstr(json, "\"type\":\"metrics\"") || strstr(json, "\"type\": \"metrics\"")) return MSG_METRICS;
    if (strstr(json, "\"type\":\"add_rule\"") || strstr(json, "\"type\": \"add_rule\"")) return MSG_ADD_RULE;
    if (strstr(json, "\"type\":\"reload\"") || strstr(json, "\"type\": \"reload\"")) return MSG_RELOAD;
    return MSG_UNKNOWN;
}

int protocol_parse_request(const char *json, parsed_request_t *req) {
    const char *id_start, *id_end, *q_start, *q_end;
    char idbuf[32];

    if (!json || !req) return -1;
    memset(req, 0, sizeof(*req));

    id_start = strstr(json, "\"id\"");
    if (id_start) {
        id_start = strchr(id_start, ':');
        if (id_start) {
            id_start++;
            while (*id_start == ' ') id_start++;
            id_end = id_start;
            while (*id_end && *id_end != ',' && *id_end != '}' && *id_end != ' ') id_end++;
            size_t idlen = (size_t)(id_end - id_start);
            if (idlen >= sizeof(idbuf)) idlen = sizeof(idbuf) - 1;
            memcpy(idbuf, id_start, idlen); idbuf[idlen] = '\0';
            req->id = (uint64_t)atol(idbuf);
        }
    }

    req->type = MSG_REQUEST;
    q_start = strstr(json, "\"query\"");
    if (!q_start) return -1;
    q_start = strchr(q_start, ':');
    if (!q_start) return -1;
    q_start++;
    while (*q_start == ' ' || *q_start == '"') q_start++;
    q_end = strchr(q_start, '"');
    if (!q_end) q_end = q_start + strlen(q_start);
    size_t qlen = (size_t)(q_end - q_start);
    if (qlen >= sizeof(req->query)) qlen = sizeof(req->query) - 1;
    memcpy(req->query, q_start, qlen); req->query[qlen] = '\0';
    return 0;
}

int protocol_parse_ping(const char *json) { (void)json; return 0; }

int protocol_parse_register(const char *json, char *name, size_t name_sz) {
    const char *n_start, *n_end;
    if (!json || !name) return -1;
    n_start = strstr(json, "\"name\"");
    if (!n_start) return -1;
    n_start = strchr(n_start, ':');
    if (!n_start) return -1;
    n_start++;
    while (*n_start == ' ' || *n_start == '"') n_start++;
    n_end = strchr(n_start, '"');
    if (!n_end) return -1;
    size_t nlen = (size_t)(n_end - n_start);
    if (nlen >= name_sz) nlen = name_sz - 1;
    memcpy(name, n_start, nlen); name[nlen] = '\0';
    return 0;
}

int protocol_parse_add_rule(const char *json, char *type, size_t type_sz, char *data, size_t data_sz) {
    const char *t_start, *t_end, *d_start, *d_end;
    int count = 0;

    if (!json || !type || !data) return -1;

    t_start = strstr(json, "\"rule_type\"");
    if (t_start) {
        t_start = strchr(t_start, ':');
        if (t_start) {
            t_start++;
            while (*t_start == ' ' || *t_start == '"') t_start++;
            t_end = strchr(t_start, '"');
            if (t_end) {
                size_t tlen = (size_t)(t_end - t_start);
                if (tlen >= type_sz) tlen = type_sz - 1;
                memcpy(type, t_start, tlen); type[tlen] = '\0';
                count++;
            }
        }
    }

    d_start = strstr(json, "\"data\"");
    if (d_start) {
        d_start = strchr(d_start, ':');
        if (d_start) {
            d_start++;
            while (*d_start == ' ' || *d_start == '"') d_start++;
            d_end = strchr(d_start, '"');
            if (d_end) {
                size_t dlen = (size_t)(d_end - d_start);
                if (dlen >= data_sz) dlen = data_sz - 1;
                memcpy(data, d_start, dlen); data[dlen] = '\0';
                count++;
            }
        }
    }

    return (count >= 2) ? 0 : -1;
}

int protocol_build_response(const match_result_t *result, uint64_t id, char *buf, size_t bufsz) {
    if (!result || !buf) return -1;
    if (result->intent == INTENT_UNKNOWN)
        return snprintf(buf, bufsz, "{\"id\":%lu,\"type\":\"response\",\"intent\":\"unknown\",\"confidence\":0.0,\"source\":\"none\"}", (unsigned long)id);

    return snprintf(buf, bufsz,
        "{\"id\":%lu,\"type\":\"response\",\"intent\":\"%s\",\"target\":\"%s\",\"action\":\"%s\",\"confidence\":%.2f,\"source\":\"%s\",\"matched\":\"%s\"}",
        (unsigned long)id, intent_type_str(result->intent), result->target, result->action,
        result->confidence,
        (result->source == MATCH_SOURCE_CACHE)  ? "cache"  :
        (result->source == MATCH_SOURCE_REGEX)  ? "regex"  :
        (result->source == MATCH_SOURCE_TOKEN)  ? "token"  :
        (result->source == MATCH_SOURCE_INTENT) ? "intent" :
        (result->source == MATCH_SOURCE_ALIAS)  ? "alias"  :
        (result->source == MATCH_SOURCE_FUZZY)  ? "fuzzy"  : "none",
        result->matched_pattern);
}

int protocol_build_multi_response(const match_result_t *results, int count, uint64_t id, char *buf, size_t bufsz) {
    size_t pos = 0;
    pos += (size_t)snprintf(buf + pos, bufsz - pos, "{\"id\":%lu,\"type\":\"response\",\"multi\":true,\"results\":[", (unsigned long)id);
    for (int i = 0; i < count && pos < bufsz; i++) {
        pos += (size_t)snprintf(buf + pos, bufsz - pos,
            "%s{\"intent\":\"%s\",\"target\":\"%s\",\"action\":\"%s\",\"confidence\":%.2f,\"source\":\"%s\"}",
            i > 0 ? "," : "", intent_type_str(results[i].intent), results[i].target,
            results[i].action, results[i].confidence,
            (results[i].source == MATCH_SOURCE_CACHE)  ? "cache"  :
            (results[i].source == MATCH_SOURCE_REGEX)  ? "regex"  :
            (results[i].source == MATCH_SOURCE_TOKEN)  ? "token"  :
            (results[i].source == MATCH_SOURCE_INTENT) ? "intent" :
            (results[i].source == MATCH_SOURCE_ALIAS)  ? "alias"  :
            (results[i].source == MATCH_SOURCE_FUZZY)  ? "fuzzy"  : "none");
    }
    snprintf(buf + pos, bufsz - pos, "]}");
    return (int)strlen(buf);
}

int protocol_build_error(uint64_t id, const char *error_code, char *buf, size_t bufsz) {
    return snprintf(buf, bufsz, "{\"id\":%lu,\"type\":\"error\",\"error_code\":\"%s\"}", (unsigned long)id, error_code);
}

int protocol_build_ping_response(char *buf, size_t bufsz) {
    return snprintf(buf, bufsz, "{\"type\":\"response\",\"status\":\"alive\"}");
}

int protocol_build_version_response(char *buf, size_t bufsz, time_t uptime, int cache_size) {
    return snprintf(buf, bufsz,
        "{\"type\":\"response\",\"version\":\"%d.%d.%d\",\"uptime\":%ld,\"cache_size\":%d,\"tier\":0}",
        VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, (long)(time(NULL) - uptime), cache_size);
}

int protocol_build_metrics_response(char *buf, size_t bufsz, const pipeline_metrics_t *m) {
    time_t uptime = time(NULL) - m->uptime_start;
    return snprintf(buf, bufsz,
        "{\"type\":\"response\",\"metrics\":{"
        "\"total_requests\":%lu,\"cache_hits\":%lu,\"regex_hits\":%lu,"
        "\"token_hits\":%lu,\"intent_hits\":%lu,\"alias_hits\":%lu,"
        "\"fuzzy_hits\":%lu,\"unknown_hits\":%lu,\"multi_intents\":%lu,"
        "\"cache_hit_ratio\":%.2f,\"avg_latency_us\":%.2f,\"max_latency_us\":%.2f,"
        "\"uptime_sec\":%ld,\"active_connections\":%lu,\"cache_size\":%lu}}",
        (unsigned long)m->total_requests, (unsigned long)m->cache_hits,
        (unsigned long)m->regex_hits, (unsigned long)m->token_hits,
        (unsigned long)m->intent_hits, (unsigned long)m->alias_hits,
        (unsigned long)m->fuzzy_hits, (unsigned long)m->unknown_hits,
        (unsigned long)m->multi_intents,
        m->total_requests > 0 ? (double)m->cache_hits / (double)m->total_requests : 0.0,
        m->avg_latency_us, m->max_latency_us,
        (long)uptime, (unsigned long)m->active_connections, (unsigned long)m->cache_size);
}
