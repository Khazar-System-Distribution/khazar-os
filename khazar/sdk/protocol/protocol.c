#include "protocol.h"
#include "../json/json.h"
#include "../logger/logger.h"
#include <stdio.h>
#include <string.h>

#define MODULE "protocol"

int protocol_parse_type(const char *raw, size_t len, message_type_t *type) {
    (void)len;
    if (!raw || !type) return -1;
    *type = MSG_UNKNOWN;

    if (strstr(raw, "\"type\":\"ping\""))          { *type = MSG_PING;      return 0; }
    if (strstr(raw, "\"type\":\"pong\""))          { *type = MSG_PONG;      return 0; }
    if (strstr(raw, "\"type\":\"heartbeat\""))    { *type = MSG_HEARTBEAT; return 0; }
    if (strstr(raw, "\"type\":\"event\""))         { *type = MSG_EVENT;     return 0; }
    if (strstr(raw, "\"type\":\"register\""))     { *type = MSG_REGISTER;  return 0; }
    if (strstr(raw, "\"type\":\"unregister\""))   { *type = MSG_UNREGISTER;return 0; }
    if (strstr(raw, "\"type\":\"response\""))     { *type = MSG_RESPONSE;  return 0; }
    if (strstr(raw, "\"type\":\"request\"") || strstr(raw, "\"query\"")) {
        *type = MSG_REQUEST;
        return 0;
    }

    return -1;
}

int protocol_parse_request(const char *raw, size_t len, request_t *req) {
    (void)len;
    if (!raw || !req) return -1;

    memset(req, 0, sizeof(*req));

    json_extract_string(raw, "query", req->query, sizeof(req->query));

    long long id_val = 0;
    if (json_extract_int(raw, "id", &id_val) == 0) {
        req->id = (uint64_t)id_val;
    }

    return 0;
}

int protocol_parse_intent(const char *raw, size_t len, intent_t *intent, float *confidence) {
    (void)len;
    if (!raw || !intent) return -1;

    memset(intent, 0, sizeof(*intent));

    json_extract_string(raw, "intent", intent->required_capability,
                        sizeof(intent->required_capability));
    json_extract_string(raw, "target", intent->target, sizeof(intent->target));
    json_extract_string(raw, "action", intent->action, sizeof(intent->action));

    if (confidence) {
        double conf_val = 0.0;
        if (json_extract_double(raw, "confidence", &conf_val) == 0) {
            *confidence = (float)conf_val;
        }
    }

    return 0;
}

int protocol_parse_register(const char *raw, size_t len, agent_info_t *agent) {
    (void)len;
    if (!raw || !agent) return -1;

    memset(agent, 0, sizeof(*agent));

    json_extract_string(raw, "name", agent->name, sizeof(agent->name));
    json_extract_string(raw, "version", agent->version, sizeof(agent->version));

    json_extract_string_array(raw, "capabilities", agent->capabilities,
                              MAX_CAPABILITIES, &agent->cap_count);

    return 0;
}

int protocol_build_response(const response_t *resp, char *out, size_t out_len) {
    const char *status_str = resp->status == STATUS_SUCCESS ? "success" : "error";

    if (resp->error_code != ERR_NONE) {
        return snprintf(out, out_len,
            "{\"id\":%llu,\"status\":\"%s\",\"error_code\":\"%s\",\"payload\":{\"message\":\"%s\"}}",
            (unsigned long long)resp->id, status_str,
            error_code_str(resp->error_code), resp->payload);
    }

    return snprintf(out, out_len,
        "{\"id\":%llu,\"status\":\"%s\",\"payload\":{\"message\":\"%s\"}}",
        (unsigned long long)resp->id, status_str, resp->payload);
}

int protocol_build_ping(char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"ping\"}");
}

int protocol_build_register(const agent_info_t *agent, char *out, size_t out_len) {
    char caps_buf[2048] = "";
    for (int i = 0; i < agent->cap_count; i++) {
        char cap_entry[72];
        snprintf(cap_entry, sizeof(cap_entry), "%s\"%s\"",
                 i > 0 ? "," : "", agent->capabilities[i]);
        strncat(caps_buf, cap_entry, sizeof(caps_buf) - strlen(caps_buf) - 1);
    }

    return snprintf(out, out_len,
        "{\"type\":\"register\",\"name\":\"%s\",\"version\":\"%s\",\"capabilities\":[%s]}",
        agent->name, agent->version, caps_buf);
}

int protocol_build_heartbeat(const char *agent_name, char *out, size_t out_len) {
    return snprintf(out, out_len,
        "{\"type\":\"heartbeat\",\"name\":\"%s\",\"timestamp\":%ld}",
        agent_name, (long)time(NULL));
}

int protocol_build_event(const char *event_type, const char *data, char *out, size_t out_len) {
    return snprintf(out, out_len,
        "{\"type\":\"event\",\"event_type\":\"%s\",\"data\":%s}",
        event_type, data ? data : "null");
}

int protocol_build_error(uint64_t id, error_code_t code, const char *msg,
                         char *out, size_t out_len) {
    return snprintf(out, out_len,
        "{\"id\":%llu,\"status\":\"error\",\"error_code\":\"%s\",\"payload\":{\"message\":\"%s\"}}",
        (unsigned long long)id, error_code_str(code), msg ? msg : "");
}
