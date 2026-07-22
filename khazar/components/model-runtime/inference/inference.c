#include "inference.h"
#include "logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MODULE "inference"

struct inference_engine {
    char               model_path[MAX_MODEL_PATH_LEN];
    inference_backend_t backend;
    model_state_t      state;
    int                context_size;
    int                num_threads;
    char               model_name[128];
    time_t             loaded_at;
};

static const char *mock_templates[] = {
    "open_application|firefox|aç",
    "open_application|chromium|aç",
    "open_application|telegram|aç",
    "open_application|vlc|aç",
    "open_application|spotify|aç",
    "open_application|discord|aç",
    "open_application|gimp|aç",
    "open_application|blender|aç",
    "open_application|obs|aç",
    "install_package|steam|quraşdır",
    "install_package|vlc|quraşdır",
    "install_package|discord|quraşdır",
    "remove_package|steam|sil",
    "search_package|vlc|ara",
    "system_shutdown||söndür",
    "system_reboot||yenidən başlat",
    "system_lock||kilidlə",
    "system_suspend||yuxu",
    "network_enable|wifi|aç",
    "network_disable|wifi|bağla",
    "volume_up||artır",
    "volume_down||azalt",
    "volume_mute||səssiz",
    "brightness_up||artır",
    "brightness_down||azalt",
    "screenshot||çək",
    "file_open|documents|aç",
    "file_search|config|tap",
    "system_update||yenilə",
    "process_kill|firefox|bağla",
};

static int mock_count = sizeof(mock_templates) / sizeof(mock_templates[0]);

static int mock_generate(const inference_request_t *req, inference_response_t *resp) {
    resp->id = req->id;
    resp->success = true;

    char query_lower[MAX_PROMPT_LEN];
    int qi = 0;
    for (const char *p = req->prompt; *p && qi < (int)sizeof(query_lower) - 1; p++)
        query_lower[qi++] = (*p >= 'A' && *p <= 'Z') ? *p + 32 : *p;
    query_lower[qi] = '\0';

    int best_idx = -1;
    int best_score = 0;

    for (int i = 0; i < mock_count; i++) {
        char tmpl[512];
        strncpy(tmpl, mock_templates[i], sizeof(tmpl) - 1);

        char *pipe1 = strchr(tmpl, '|');
        char *pipe2 = pipe1 ? strchr(pipe1 + 1, '|') : NULL;
        if (!pipe1 || !pipe2) continue;

        *pipe1 = '\0'; *pipe2 = '\0';
        char *intent_str = tmpl;
        char *target_str = pipe1 + 1;
        char *action_str = pipe2 + 1;

        int score = 0;
        if (strstr(query_lower, target_str) && target_str[0]) score += 3;
        if (strstr(query_lower, action_str) && action_str[0]) score += 2;
        if (strstr(query_lower, intent_str)) score += 1;

        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    if (best_idx >= 0 && best_score > 0) {
        char tmpl[512];
        strncpy(tmpl, mock_templates[best_idx], sizeof(tmpl) - 1);
        char *pipe1 = strchr(tmpl, '|');
        char *pipe2 = pipe1 ? strchr(pipe1 + 1, '|') : NULL;
        *pipe1 = '\0'; *pipe2 = '\0';

        snprintf(resp->generated_text, sizeof(resp->generated_text),
            "{\"intent\":\"%s\",\"target\":\"%s\",\"action\":\"%s\",\"confidence\":0.85}",
            tmpl, pipe1 + 1, pipe2 + 1);
    } else {
        snprintf(resp->generated_text, sizeof(resp->generated_text),
            "{\"intent\":\"unknown\",\"confidence\":0.0}");
    }

    resp->tokens_generated = (int)strlen(resp->generated_text) / 3;
    resp->tokens_per_second = 999.0f;

    return 0;
}

inference_engine_t *inference_init(const char *model_path, const char *backend,
                                    int context_size, int num_threads) {
    inference_engine_t *eng = calloc(1, sizeof(inference_engine_t));
    if (!eng) return NULL;

    if (model_path) strncpy(eng->model_path, model_path, sizeof(eng->model_path) - 1);
    eng->context_size = context_size > 0 ? context_size : 2048;
    eng->num_threads = num_threads > 0 ? num_threads : 4;
    eng->state = MODEL_STATE_UNLOADED;

    if (backend && strcmp(backend, "llamacpp") == 0) {
        eng->backend = INFERENCE_BACKEND_LLAMACPP;
    } else {
        eng->backend = INFERENCE_BACKEND_MOCK;
        log_info(MODULE, "using MOCK backend for development");
    }

    log_info(MODULE, "inference engine initialised (backend=%s)", backend ? backend : "mock");
    return eng;
}

int inference_load_model(inference_engine_t *eng) {
    if (!eng) return -1;

    eng->state = MODEL_STATE_LOADING;

    if (eng->backend == INFERENCE_BACKEND_MOCK) {
        strncpy(eng->model_name, "mock-gguf-model-v1", sizeof(eng->model_name) - 1);
        eng->loaded_at = time(NULL);
        eng->state = MODEL_STATE_LOADED;
        log_info(MODULE, "mock model loaded (%.1f KB simulated)", 3400.0f);
        return 0;
    }

    eng->state = MODEL_STATE_ERROR;
    log_error(MODULE, "llamacpp backend requires llama.cpp library - using mock instead");
    eng->backend = INFERENCE_BACKEND_MOCK;
    return inference_load_model(eng);
}

int inference_generate(inference_engine_t *eng,
                       const inference_request_t *req,
                       inference_response_t *resp) {
    if (!eng || !req || !resp) return -1;

    if (eng->state != MODEL_STATE_LOADED) {
        memset(resp, 0, sizeof(*resp));
        resp->id = req->id;
        resp->success = false;
        snprintf(resp->error_message, sizeof(resp->error_message), "model not loaded");
        return -1;
    }

    memset(resp, 0, sizeof(*resp));
    resp->success = false;

    if (eng->backend == INFERENCE_BACKEND_MOCK) {
        return mock_generate(req, resp);
    }

    snprintf(resp->error_message, sizeof(resp->error_message), "backend not implemented");
    return -1;
}

model_state_t inference_get_state(inference_engine_t *eng) {
    return eng ? eng->state : MODEL_STATE_ERROR;
}

const char *inference_get_model_name(inference_engine_t *eng) {
    return eng ? eng->model_name : NULL;
}

void inference_unload_model(inference_engine_t *eng) {
    if (!eng) return;
    eng->state = MODEL_STATE_UNLOADED;
    log_info(MODULE, "model unloaded");
}

void inference_cleanup(inference_engine_t *eng) {
    if (!eng) return;
    inference_unload_model(eng);
    free(eng);
    log_info(MODULE, "inference engine cleaned up");
}
