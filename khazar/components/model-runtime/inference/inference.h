#ifndef INFERENCE_H
#define INFERENCE_H

#include "../include/common.h"

typedef struct inference_engine inference_engine_t;

inference_engine_t *inference_init(const char *model_path, const char *backend,
                                    int context_size, int num_threads);
int                 inference_load_model(inference_engine_t *eng);
int                 inference_generate(inference_engine_t *eng,
                                       const inference_request_t *req,
                                       inference_response_t *resp);
model_state_t       inference_get_state(inference_engine_t *eng);
const char         *inference_get_model_name(inference_engine_t *eng);
void                inference_unload_model(inference_engine_t *eng);
void                inference_cleanup(inference_engine_t *eng);

#endif
