#ifndef AI_SDK_PROTOCOL_H
#define AI_SDK_PROTOCOL_H

#include "../include/common.h"

typedef enum {
    MSG_UNKNOWN,
    MSG_REQUEST,
    MSG_RESPONSE,
    MSG_PING,
    MSG_PONG,
    MSG_REGISTER,
    MSG_UNREGISTER,
    MSG_HEARTBEAT,
    MSG_EVENT,
    MSG_ERROR
} message_type_t;

int protocol_parse_type(const char *raw, size_t len, message_type_t *type);

int protocol_parse_request(const char *raw, size_t len, request_t *req);

int protocol_parse_intent(const char *raw, size_t len, intent_t *intent, float *confidence);

int protocol_parse_register(const char *raw, size_t len, agent_info_t *agent);

int protocol_build_response(const response_t *resp, char *out, size_t out_len);

int protocol_build_ping(char *out, size_t out_len);

int protocol_build_register(const agent_info_t *agent, char *out, size_t out_len);

int protocol_build_heartbeat(const char *agent_name, char *out, size_t out_len);

int protocol_build_event(const char *event_type, const char *data, char *out, size_t out_len);

int protocol_build_error(uint64_t id, error_code_t code, const char *msg,
                         char *out, size_t out_len);

#endif
