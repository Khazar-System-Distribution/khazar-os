#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../include/common.h"

typedef enum {
    CMSG_UNKNOWN = 0,
    CMSG_CLASSIFY,
    CMSG_PING,
    CMSG_VERSION,
    CMSG_RELOAD
} classifier_message_type_t;

int protocol_parse_type(const char *raw, size_t len, classifier_message_type_t *type);
int protocol_parse_classify(const char *raw, size_t len, classifier_request_t *req);
int protocol_build_classify_response(const classifier_response_t *resp, char *out, size_t out_len);
int protocol_build_ping(char *out, size_t out_len);
int protocol_build_version(char *out, size_t out_len);
int protocol_build_error(const char *msg, char *out, size_t out_len);

#endif
