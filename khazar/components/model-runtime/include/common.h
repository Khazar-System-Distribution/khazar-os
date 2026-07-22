#ifndef COMMON_H
#define COMMON_H

#include "../../../sdk/include/common.h"
#include <string.h>

#define SOCKET_PATH_DEFAULT        "/run/ai-model-runtime.sock"
#define MAX_PROMPT_LEN             4096
#define MAX_GENERATION_LEN         8192
#define MAX_MODEL_PATH_LEN         512
#define DEFAULT_MAX_TOKENS         256
#define DEFAULT_TEMPERATURE        0.7f
#define DEFAULT_TOP_P              0.9f
#define DEFAULT_MODEL_PATH         "/usr/share/ai-models/default.gguf"

typedef enum {
    INFERENCE_BACKEND_NONE,
    INFERENCE_BACKEND_LLAMACPP,
    INFERENCE_BACKEND_MOCK
} inference_backend_t;

typedef enum {
    MODEL_STATE_UNLOADED,
    MODEL_STATE_LOADING,
    MODEL_STATE_LOADED,
    MODEL_STATE_ERROR
} model_state_t;

typedef struct {
    uint64_t    id;
    char        prompt[MAX_PROMPT_LEN];
    int         max_tokens;
    float       temperature;
    float       top_p;
    bool        stream;
    char        system_prompt[MAX_PROMPT_LEN];
} inference_request_t;

typedef struct {
    uint64_t    id;
    char        generated_text[MAX_GENERATION_LEN];
    int         tokens_generated;
    float       tokens_per_second;
    bool        success;
    char        error_message[256];
} inference_response_t;

typedef struct {
    char             path[MAX_MODEL_PATH_LEN];
    inference_backend_t backend;
    model_state_t    state;
    char             model_name[128];
    int              context_size;
    time_t           loaded_at;
} model_info_t;

#endif
