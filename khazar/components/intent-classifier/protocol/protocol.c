#include "protocol.h"
#include "logger/logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MODULE "protocol"

int protocol_parse_type(const char *raw, size_t len, classifier_message_type_t *type) {
    (void)len;
    if (!raw || !type) return -1;
    *type = CMSG_UNKNOWN;

    if (strstr(raw, "\"type\":\"ping\""))          { *type = CMSG_PING;     return 0; }
    if (strstr(raw, "\"type\":\"version\""))       { *type = CMSG_VERSION;  return 0; }
    if (strstr(raw, "\"type\":\"reload\""))        { *type = CMSG_RELOAD;   return 0; }
    if (strstr(raw, "\"type\":\"classify\"") || strstr(raw, "\"query\"")) {
        *type = CMSG_CLASSIFY; return 0;
    }

    return 0;
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
    while (*p && *p != '"' && i < (int)bufsz - 1) {
        if (*p == '\\' && *(p + 1)) p++;
        buf[i++] = *p++;
    }
    buf[i] = '\0';
    return i;
}

int protocol_parse_classify(const char *raw, size_t len, classifier_request_t *req) {
    (void)len;
    if (!raw || !req) return -1;
    memset(req, 0, sizeof(*req));

    const char *idp = strstr(raw, "\"id\"");
    if (idp) {
        idp = strchr(idp, ':');
        if (idp) req->id = strtoull(idp + 1, NULL, 10);
    }

    extract_str(raw, "query", req->query, sizeof(req->query));

    return 0;
}

int protocol_build_classify_response(const classifier_response_t *resp, char *out, size_t out_len) {
    char escaped_target[MAX_TARGET_LEN * 2];
    int ei = 0;
    for (const char *p = resp->target; *p && ei < (int)sizeof(escaped_target) - 2; p++) {
        if (*p == '"') { escaped_target[ei++] = '\\'; escaped_target[ei++] = '"'; }
        else if (*p == '\\') { escaped_target[ei++] = '\\'; escaped_target[ei++] = '\\'; }
        else escaped_target[ei++] = *p;
    }
    escaped_target[ei] = '\0';

    char escaped_intent[MAX_INTENT_LEN * 2];
    ei = 0;
    for (const char *p = resp->intent; *p && ei < (int)sizeof(escaped_intent) - 2; p++) {
        if (*p == '"') { escaped_intent[ei++] = '\\'; escaped_intent[ei++] = '"'; }
        else if (*p == '\\') { escaped_intent[ei++] = '\\'; escaped_intent[ei++] = '\\'; }
        else escaped_intent[ei++] = *p;
    }
    escaped_intent[ei] = '\0';

    return snprintf(out, out_len,
        "{\"id\":%llu,\"type\":\"classify_response\",\"intent\":\"%s\",\"target\":\"%s\","
        "\"confidence\":%.2f,\"fallback\":%s}",
        (unsigned long long)resp->id,
        escaped_intent,
        escaped_target,
        resp->confidence,
        resp->fallback ? "true" : "false");
}

int protocol_build_ping(char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"classify_response\",\"status\":\"alive\"}");
}

int protocol_build_version(char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"classify_response\",\"version\":\"0.1.0\",\"component\":\"ai-intent-classifier\"}");
}

int protocol_build_error(const char *msg, char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"classify_response\",\"success\":false,\"error\":\"%s\"}", msg);
}
