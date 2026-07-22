#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../include/common.h"

typedef enum {
    MSG_UNKNOWN,
    MSG_REQUEST,
    MSG_RESPONSE,
    MSG_PING,
    MSG_REGISTER
} message_type_t;

int  protocol_parse(const char *raw, size_t len, message_type_t *type, request_t *req);
int  protocol_build_response(const response_t *resp, char *out, size_t out_len);
int  protocol_build_ping_response(char *out, size_t out_len);
int  protocol_build_error(uint64_t id, error_code_t code, char *out, size_t out_len);

#endif
