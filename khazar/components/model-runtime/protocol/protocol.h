#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../include/common.h"

typedef enum {
    RMSG_UNKNOWN = 0,
    RMSG_INFERENCE,
    RMSG_PING,
    RMSG_VERSION,
    RMSG_LOAD_MODEL,
    RMSG_UNLOAD_MODEL,
    RMSG_STATUS
} runtime_message_type_t;

int protocol_parse_type(const char *raw, size_t len, runtime_message_type_t *type);
int protocol_parse_inference(const char *raw, size_t len, inference_request_t *req);
int protocol_build_inference_response(const inference_response_t *resp, char *out, size_t out_len);
int protocol_build_ping(char *out, size_t out_len);
int protocol_build_version(const model_info_t *info, char *out, size_t out_len);
int protocol_build_status(const model_info_t *info, char *out, size_t out_len);
int protocol_build_error(const char *msg, char *out, size_t out_len);

#endif
