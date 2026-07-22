#include "protocol.h"
#include "logger/logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MODULE "protocol"

int protocol_parse(const char *raw, size_t len, message_type_t *type, request_t *req) {
    (void)len;
    if (!raw || !type) return -1;

    *type = MSG_UNKNOWN;

    if (strstr(raw, "\"type\":\"ping\"")) {
        *type = MSG_PING;
        return 0;
    }

    if (strstr(raw, "\"type\":\"request\"") || strstr(raw, "\"query\"")) {
        *type = MSG_REQUEST;
        if (req) {
            memset(req, 0, sizeof(*req));

            const char *q = strstr(raw, "\"query\"");
            if (q) {
                q = strchr(q, ':');
                if (q) {
                    while (*q && *q != '"') q++;
                    if (*q == '"') {
                        q++;
                        int idx = 0;
                        while (*q && *q != '"' && idx < (int)sizeof(req->query) - 1) {
                            if (*q == '\\' && *(q+1)) q++;
                            req->query[idx++] = *q++;
                        }
                        req->query[idx] = '\0';
                    }
                }
            }

            const char *idp = strstr(raw, "\"id\"");
            if (idp) {
                idp = strchr(idp, ':');
                if (idp) req->id = strtoull(idp + 1, NULL, 10);
            }
        }
        return 0;
    }

    if (strstr(raw, "\"type\":\"register\"")) {
        *type = MSG_REGISTER;
        return 0;
    }

    return -1;
}

int protocol_build_response(const response_t *resp, char *out, size_t out_len) {
    const char *status_str = resp->status == STATUS_SUCCESS ? "success" : "error";
    return snprintf(out, out_len,
        "{\"id\":%llu,\"status\":\"%s\",\"payload\":{\"message\":\"%s\"}}",
        (unsigned long long)resp->id, status_str, resp->payload);
}

int protocol_build_ping_response(char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"status\":\"alive\"}");
}

int protocol_build_error(uint64_t id, error_code_t code, char *out, size_t out_len) {
    return snprintf(out, out_len,
        "{\"id\":%llu,\"status\":\"error\",\"error_code\":\"%s\"}",
        (unsigned long long)id, error_code_str(code));
}
