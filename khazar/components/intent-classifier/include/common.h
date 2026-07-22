#ifndef COMMON_H
#define COMMON_H

#include "../../../sdk/include/common.h"
#include <string.h>

#define SOCKET_PATH_DEFAULT           "/run/ai-intent-classifier.sock"
#define RUNTIME_SOCKET_DEFAULT        "/run/ai-model-runtime.sock"
#define MAX_QUERY_LEN                 2048
#define MAX_INTENT_LEN                64
#define MAX_TARGET_LEN                128
#define MAX_RESPONSE_LEN              65536
#define MAX_PROMPT_LEN                4096
#define DEFAULT_MODEL_RUNTIME_TIMEOUT 10000

typedef struct {
    uint64_t id;
    char     query[MAX_QUERY_LEN];
} classifier_request_t;

typedef struct {
    uint64_t id;
    char     intent[MAX_INTENT_LEN];
    char     target[MAX_TARGET_LEN];
    float    confidence;
    bool     fallback;
    char     raw_output[MAX_PROMPT_LEN];
} classifier_response_t;

#endif
