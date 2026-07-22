#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"

typedef enum {
    MSG_UNKNOWN = 0,
    MSG_REQUEST,
    MSG_RESPONSE,
    MSG_PING,
    MSG_REGISTER,
    MSG_VERSION,
    MSG_METRICS,
    MSG_ADD_RULE,
    MSG_RELOAD
} message_type_t;

typedef struct {
    uint64_t    id;
    message_type_t type;
    char        query[MAX_QUERY_LEN];
} parsed_request_t;

typedef struct {
    uint64_t    id;
    char        intent[MAX_INTENT_NAME_LEN];
    char        target[MAX_TARGET_LEN];
    char        action[MAX_ACTION_LEN];
    float       confidence;
    char        source[32];
    char        matched[128];
} parsed_response_t;

message_type_t protocol_parse_type(const char *json);
int  protocol_parse_request(const char *json, parsed_request_t *req);
int  protocol_parse_ping(const char *json);
int  protocol_parse_register(const char *json, char *name, size_t name_sz);
int  protocol_parse_add_rule(const char *json, char *type, size_t type_sz, char *data, size_t data_sz);
int  protocol_build_response(const match_result_t *result, uint64_t id, char *buf, size_t bufsz);
int  protocol_build_multi_response(const match_result_t *results, int count, uint64_t id, char *buf, size_t bufsz);
int  protocol_build_error(uint64_t id, const char *error_code, char *buf, size_t bufsz);
int  protocol_build_ping_response(char *buf, size_t bufsz);
int  protocol_build_version_response(char *buf, size_t bufsz, time_t uptime, int cache_size);
int  protocol_build_metrics_response(char *buf, size_t bufsz, const pipeline_metrics_t *m);

#endif
