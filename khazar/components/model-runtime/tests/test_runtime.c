#include "../include/common.h"
#include "../inference/inference.h"
#include "../protocol/protocol.h"
#include <stdio.h>
#include <string.h>

static int test_inference_mock(void) {
    inference_engine_t *eng = inference_init("/tmp/test.gguf", "mock", 2048, 1);
    if (!eng) { fprintf(stderr, "FAIL: init\n"); return 1; }
    if (inference_load_model(eng) != 0) { fprintf(stderr, "FAIL: load\n"); inference_cleanup(eng); return 1; }
    if (inference_get_state(eng) != MODEL_STATE_LOADED) { fprintf(stderr, "FAIL: state\n"); inference_cleanup(eng); return 1; }

    inference_request_t req = {.id = 1, .max_tokens = 128, .temperature = 0.7f};
    strcpy(req.prompt, "firefox ac");
    inference_response_t resp;
    if (inference_generate(eng, &req, &resp) != 0) { fprintf(stderr, "FAIL: generate\n"); inference_cleanup(eng); return 1; }
    if (!resp.success) { fprintf(stderr, "FAIL: success flag\n"); inference_cleanup(eng); return 1; }
    if (!strstr(resp.generated_text, "intent")) { fprintf(stderr, "FAIL: no intent in output\n"); inference_cleanup(eng); return 1; }

    printf("PASS: inference_mock\n");
    inference_cleanup(eng);
    return 0;
}

static int test_inference_unknown(void) {
    inference_engine_t *eng = inference_init(NULL, "mock", 2048, 1);
    inference_load_model(eng);
    inference_request_t req = {.id = 2, .max_tokens = 128};
    strcpy(req.prompt, "xyzabc123");
    inference_response_t resp;
    inference_generate(eng, &req, &resp);
    if (!strstr(resp.generated_text, "unknown")) {
        fprintf(stderr, "FAIL: should be unknown for gibberish\n");
        inference_cleanup(eng); return 1;
    }
    printf("PASS: inference_unknown\n");
    inference_cleanup(eng);
    return 0;
}

static int test_protocol_build_response(void) {
    inference_response_t resp = {.id = 42, .success = true, .tokens_generated = 50, .tokens_per_second = 100.0f};
    strcpy(resp.generated_text, "{\"intent\":\"open_application\",\"target\":\"firefox\"}");
    char out[4096];
    protocol_build_inference_response(&resp, out, sizeof(out));
    if (!strstr(out, "\"success\":true") || !strstr(out, "open_application")) {
        fprintf(stderr, "FAIL: build response: %s\n", out);
        return 1;
    }
    printf("PASS: protocol_build_response\n");
    return 0;
}

static int test_protocol_parse_inference(void) {
    const char *json = "{\"type\":\"inference\",\"id\":7,\"prompt\":\"hello world\",\"max_tokens\":128,\"temperature\":0.5}";
    inference_request_t req;
    protocol_parse_inference(json, strlen(json), &req);
    if (strcmp(req.prompt, "hello world") != 0) { fprintf(stderr, "FAIL: prompt\n"); return 1; }
    if (req.max_tokens != 128) { fprintf(stderr, "FAIL: max_tokens=%d\n", req.max_tokens); return 1; }
    printf("PASS: protocol_parse_inference\n");
    return 0;
}

int main(void) {
    int f = 0;
    f += test_inference_mock();
    f += test_inference_unknown();
    f += test_protocol_build_response();
    f += test_protocol_parse_inference();
    if (f) { fprintf(stderr, "\n%d FAILED\n", f); return 1; }
    printf("\nAll Model Runtime tests passed\n");
    return 0;
}
