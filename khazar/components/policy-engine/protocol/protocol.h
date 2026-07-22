#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../include/common.h"

typedef enum {
    PMSG_UNKNOWN = 0,
    PMSG_POLICY_CHECK,
    PMSG_POLICY_RESPONSE,
    PMSG_PING,
    PMSG_VERSION,
    PMSG_RELOAD,
    PMSG_ADD_RULE
} policy_message_type_t;

int protocol_parse(const char *raw, size_t len, policy_message_type_t *type);
int protocol_parse_policy_check(const char *raw, size_t len, policy_request_t *req);
int protocol_parse_add_rule(const char *raw, size_t len, policy_rule_t *rule);
int protocol_build_response(const policy_response_t *resp, char *out, size_t out_len);
int protocol_build_ping(char *out, size_t out_len);
int protocol_build_version(char *out, size_t out_len);
int protocol_build_error(const char *msg, char *out, size_t out_len);
int protocol_build_rule_added(const char *name, char *out, size_t out_len);

#endif
