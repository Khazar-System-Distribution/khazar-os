#include "protocol.h"
#include "logger/logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MODULE "protocol"

static int json_extract_str(const char *json, const char *key, char *out, size_t out_len) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p && *p != '"') p++;
    if (*p != '"') return 0;
    p++;

    int i = 0;
    while (*p && *p != '"' && i < (int)out_len - 1) {
        if (*p == '\\' && *(p + 1)) p++;
        out[i++] = *p++;
    }
    out[i] = '\0';
    return i;
}

int protocol_parse(const char *raw, size_t len, policy_message_type_t *type) {
    (void)len;
    if (!raw || !type) return -1;
    *type = PMSG_UNKNOWN;

    if (strstr(raw, "\"type\":\"ping\""))         { *type = PMSG_PING;          return 0; }
    if (strstr(raw, "\"type\":\"version\""))      { *type = PMSG_VERSION;       return 0; }
    if (strstr(raw, "\"type\":\"reload\""))       { *type = PMSG_RELOAD;        return 0; }
    if (strstr(raw, "\"type\":\"add_rule\""))     { *type = PMSG_ADD_RULE;      return 0; }
    if (strstr(raw, "\"type\":\"policy_check\"")) { *type = PMSG_POLICY_CHECK;  return 0; }

    return 0;
}

int protocol_parse_policy_check(const char *raw, size_t len, policy_request_t *req) {
    (void)len;
    if (!raw || !req) return -1;
    memset(req, 0, sizeof(*req));

    const char *idp = strstr(raw, "\"id\"");
    if (idp) {
        idp = strchr(idp, ':');
        if (idp) req->id = strtoull(idp + 1, NULL, 10);
    }

    const char *ap = strstr(raw, "\"agent\"");
    if (ap) {
        ap = strchr(ap, ':');
        if (ap) {
            while (*ap && *ap != '"') ap++;
            if (*ap == '"') {
                ap++;
                int i = 0;
                while (*ap && *ap != '"' && i < (int)sizeof(req->agent_name) - 1)
                    req->agent_name[i++] = *ap++;
            }
        }
    }

    const char *cp = strstr(raw, "\"capability\"");
    if (cp) {
        cp = strchr(cp, ':');
        if (cp) {
            while (*cp && *cp != '"') cp++;
            if (*cp == '"') {
                cp++;
                int i = 0;
                while (*cp && *cp != '"' && i < (int)sizeof(req->capability) - 1)
                    req->capability[i++] = *cp++;
            }
        }
    }

    const char *tp = strstr(raw, "\"target\"");
    if (tp) {
        tp = strchr(tp, ':');
        if (tp) {
            while (*tp && *tp != '"') tp++;
            if (*tp == '"') {
                tp++;
                int i = 0;
                while (*tp && *tp != '"' && i < (int)sizeof(req->target) - 1)
                    req->target[i++] = *tp++;
            }
        }
    }

    return 0;
}

int protocol_parse_add_rule(const char *raw, size_t len, policy_rule_t *rule) {
    (void)len;
    if (!raw || !rule) return -1;
    memset(rule, 0, sizeof(*rule));
    rule->action = POLICY_ALLOW;
    rule->priority = 10;

    char buf[256];

    if (json_extract_str(raw, "capability", buf, sizeof(buf)) > 0)
        strncpy(rule->capability, buf, sizeof(rule->capability) - 1);

    if (json_extract_str(raw, "agent", buf, sizeof(buf)) > 0)
        strncpy(rule->agent_name, buf, sizeof(rule->agent_name) - 1);

    if (json_extract_str(raw, "action", buf, sizeof(buf)) > 0)
        rule->action = policy_action_from_str(buf);

    return 0;
}

int protocol_build_response(const policy_response_t *resp, char *out, size_t out_len) {
    return snprintf(out, out_len,
        "{\"id\":%llu,\"type\":\"policy_response\",\"action\":\"%s\",\"reason\":\"%s\",\"matched_rule\":\"%s\"}",
        (unsigned long long)resp->id,
        policy_action_str(resp->action),
        resp->reason,
        resp->matched_rule);
}

int protocol_build_ping(char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"policy_response\",\"status\":\"alive\"}");
}

int protocol_build_version(char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"policy_response\",\"version\":\"0.1.0\",\"component\":\"ai-policy-engine\"}");
}

int protocol_build_error(const char *msg, char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"policy_response\",\"status\":\"error\",\"message\":\"%s\"}", msg);
}

int protocol_build_rule_added(const char *name, char *out, size_t out_len) {
    return snprintf(out, out_len, "{\"type\":\"policy_response\",\"status\":\"rule_added\",\"rule\":\"%s\"}", name);
}
