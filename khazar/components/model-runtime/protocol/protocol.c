#include "protocol.h"
#include "logger/logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MODULE "protocol"

int protocol_parse_type(const char *raw, size_t len, runtime_message_type_t *type) {
    (void)len;
    if (!raw || !type) return -1;
    *type = RMSG_UNKNOWN;

    if (strstr(raw, "\"type\":\"ping\""))         { *type = RMSG_PING;        return 0; }
    if (strstr(raw, "\"type\":\"version\""))      { *type = RMSG_VERSION;     return 0; }
    if (strstr(raw, "\"type\":\"load_model\""))   { *type = RMSG_LOAD_MODEL;  return 0; }
    if (strstr(raw, "\"type\":\"unload_model\"")) { *type = RMSG_UNLOAD_MODEL;return 0; }
    if (strstr(raw, "\"type\":\"status\""))       { *type = RMSG_STATUS;      return 0; }
    if (strstr(raw, "\"type\":\"inference\"") || strstr(raw, "\"prompt\"")) {
        *type = RMSG_INFERENCE; return 0;
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
    while (*p && *p != '"' && i < (int)bufsz - 1) { buf[i++] = *p++; }
    buf[i] = '\0';
    return i;
}

static int extract_int(const char *json, const char *key, int *out) {
    char buf[32] = {0};
    if (extract_str(json, key, buf, sizeof(buf)) < 0) {
        char search[128];
        snprintf(search, sizeof(search), "\"%s\":", key);
        const char *p = strstr(json, search);
        if (!p) return -1;
        p += strlen(search);
        while (*p == ' ' || *p == '\t') p++;
        *out = (int)strtol(p, NULL, 10);
        return 0;
    }
    *out = atoi(buf);
    return 0;
}

static int extract_float(const char *json, const char *key, float *out) {
    char buf[32] = {0};
    if (extract_str(json, key, buf, sizeof(buf)) < 0) {
        char search[128];
        snprintf(search, sizeof(search), "\"%s\":", key);
        const char *p = strstr(json, search);
        if (!p) return -1;
        p += strlen(search);
        while (*p == ' ' || *p == '\t') p++;
        *out = (float)atof(p);
        return 0;
    }
    *out = (float)atof(buf);
    return 0;
}

int protocol_parse_inference(const char *raw, size_t len, inference_request_t *req) {
    (void)len;
    if (!raw || !req) return -1;
    memset(req, 0, sizeof(*req));

    extract_str(raw, "prompt", req->prompt, sizeof(req->prompt));
    extract_str(raw, "system_prompt", req->system_prompt, sizeof(req->system_prompt));

    int mt = DEFAULT_MAX_TOKENS;
    if (extract_int(raw, "max_tokens", &mt) == 0) req->max_tokens = mt;
    else req->max_tokens = DEFAULT_MAX_TOKENS;

    float temp = DEFAULT_TEMPERATURE;
    if (extract_float(raw, "temperature", &temp) == 0) req->temperature = temp;
    else req->temperature = DEFAULT_TEMPERATURE;

    float top_p = DEFAULT_TOP_P;
    if (extract_float(raw, "top_p", &top_p) == 0) req->top_p = top_p;
    else req->top_p = DEFAULT_TOP_P;

    return 0;
}

int protocol_build_inference_response(const inference_response_t *resp, char *out, size_t out_len) {
    if (!resp->success) {
        return snprintf(out, out_len,
            "{\"id\":%llu,\"type\":\"inference_response\",\"success\":false,\"error\":\"%s\"}",
            (unsigned long long)resp->id, resp->error_message);
    }

    char escaped[MAX_GENERATION_LEN * 2];
    int ei = 0;
    for (const char *p = resp->generated_text; *p && ei < (int)sizeof(escaped) - 2; p++) {
        if (*p == '"') { escaped[ei++] = '\\'; escaped[ei++] = '"'; }
        else if (*p == '\\') { escaped[ei++] = '\\'; escaped[ei++] = '\\'; }
        else if (*p == '\n') { escaped[ei++] = '\\'; escaped[ei++] = 'n'; }
        else escaped[ei++] = *p;
    }
    escaped[ei] = '\0';

    return snprintf(out, out_len,
        "{\"id\":%llu,\"type\":\"inference_response\",\"success\":true,"
        "\"text\":\"%s\",\"tokens_generated\":%d,\"tokens_per_second\":%.1f}",
        (unsigned long long)resp->id, escaped,
        resp->tokens_generated, resp->tokens_per_second);
}

int protocol_build_ping(char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"inference_response\",\"status\":\"alive\"}");
}

int protocol_build_version(const model_info_t *info, char *out, size_t out_len) {
    return snprintf(out, out_len,
        "{\"type\":\"inference_response\",\"version\":\"0.1.0\",\"backend\":\"%s\","
        "\"model\":\"%s\",\"context_size\":%d}",
        info->backend == INFERENCE_BACKEND_MOCK ? "mock" : "llamacpp",
        info->model_name, info->context_size);
}

int protocol_build_status(const model_info_t *info, char *out, size_t out_len) {
    const char *state_str = "unknown";
    switch (info->state) {
        case MODEL_STATE_UNLOADED: state_str = "unloaded"; break;
        case MODEL_STATE_LOADING:  state_str = "loading"; break;
        case MODEL_STATE_LOADED:   state_str = "loaded"; break;
        case MODEL_STATE_ERROR:    state_str = "error"; break;
    }
    return snprintf(out, out_len,
        "{\"type\":\"inference_response\",\"status\":\"%s\",\"model\":\"%s\","
        "\"loaded_at\":%ld}",
        state_str, info->model_name, (long)info->loaded_at);
}

int protocol_build_error(const char *msg, char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"inference_response\",\"success\":false,\"error\":\"%s\"}", msg);
}
