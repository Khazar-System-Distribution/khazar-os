#include "../include/common.h"
#include "../protocol/protocol.h"
#include <stdio.h>
#include <string.h>

static int test_protocol_parse_type(void) {
    message_type_t type;
    if (protocol_parse_type("{\"type\":\"ping\"}", 16, &type) != 0 || type != MSG_PING) {
        fprintf(stderr, "FAIL: protocol_parse_type(ping)\n");
        return 1;
    }
    if (protocol_parse_type("{\"type\":\"request\"}", 19, &type) != 0 || type != MSG_REQUEST) {
        fprintf(stderr, "FAIL: protocol_parse_type(request)\n");
        return 1;
    }
    if (protocol_parse_type("{\"type\":\"register\"}", 21, &type) != 0 || type != MSG_REGISTER) {
        fprintf(stderr, "FAIL: protocol_parse_type(register)\n");
        return 1;
    }
    if (protocol_parse_type("{\"type\":\"heartbeat\"}", 22, &type) != 0 || type != MSG_HEARTBEAT) {
        fprintf(stderr, "FAIL: protocol_parse_type(heartbeat)\n");
        return 1;
    }
    printf("PASS: protocol_parse_type\n");
    return 0;
}

static int test_protocol_parse_request(void) {
    const char *json = "{\"id\":123,\"type\":\"request\",\"query\":\"firefox ac\"}";
    request_t req;
    if (protocol_parse_request(json, strlen(json), &req) != 0) {
        fprintf(stderr, "FAIL: protocol_parse_request returned error\n");
        return 1;
    }
    if (strcmp(req.query, "firefox ac") != 0) {
        fprintf(stderr, "FAIL: protocol_parse_request query: '%s'\n", req.query);
        return 1;
    }
    if (req.id != 123) {
        fprintf(stderr, "FAIL: protocol_parse_request id: %llu\n", (unsigned long long)req.id);
        return 1;
    }
    printf("PASS: protocol_parse_request\n");
    return 0;
}

static int test_protocol_parse_intent(void) {
    const char *json = "{\"intent\":\"open_application\",\"target\":\"firefox\",\"action\":\"ac\",\"confidence\":0.95}";
    intent_t intent;
    float confidence = 0.0f;
    if (protocol_parse_intent(json, strlen(json), &intent, &confidence) != 0) {
        fprintf(stderr, "FAIL: protocol_parse_intent returned error\n");
        return 1;
    }
    if (strcmp(intent.required_capability, "open_application") != 0) {
        fprintf(stderr, "FAIL: protocol_parse_intent capability: '%s'\n", intent.required_capability);
        return 1;
    }
    if (strcmp(intent.target, "firefox") != 0) {
        fprintf(stderr, "FAIL: protocol_parse_intent target: '%s'\n", intent.target);
        return 1;
    }
    if (confidence < 0.94f || confidence > 0.96f) {
        fprintf(stderr, "FAIL: protocol_parse_intent confidence: %f\n", confidence);
        return 1;
    }
    printf("PASS: protocol_parse_intent\n");
    return 0;
}

static int test_protocol_build_response(void) {
    response_t resp = {.id = 42, .status = STATUS_SUCCESS};
    strcpy(resp.payload, "task completed");
    char out[512];
    protocol_build_response(&resp, out, sizeof(out));
    if (!strstr(out, "\"id\":42") || !strstr(out, "\"status\":\"success\"")) {
        fprintf(stderr, "FAIL: protocol_build_response: '%s'\n", out);
        return 1;
    }
    printf("PASS: protocol_build_response\n");
    return 0;
}

static int test_protocol_build_ping(void) {
    char out[128];
    protocol_build_ping(out, sizeof(out));
    if (!strstr(out, "ping")) {
        fprintf(stderr, "FAIL: protocol_build_ping: '%s'\n", out);
        return 1;
    }
    printf("PASS: protocol_build_ping\n");
    return 0;
}

static int test_protocol_build_error(void) {
    char out[256];
    protocol_build_error(7, ERR_TIMEOUT, "timeout occurred", out, sizeof(out));
    if (!strstr(out, "\"id\":7") || !strstr(out, "TIMEOUT")) {
        fprintf(stderr, "FAIL: protocol_build_error: '%s'\n", out);
        return 1;
    }
    printf("PASS: protocol_build_error\n");
    return 0;
}

static int test_protocol_build_register(void) {
    agent_info_t agent;
    memset(&agent, 0, sizeof(agent));
    strcpy(agent.name, "test-agent");
    strcpy(agent.version, "1.0.0");
    strcpy(agent.capabilities[0], "open_app");
    strcpy(agent.capabilities[1], "search");
    agent.cap_count = 2;

    char out[512];
    protocol_build_register(&agent, out, sizeof(out));
    if (!strstr(out, "test-agent") || !strstr(out, "open_app") || !strstr(out, "register")) {
        fprintf(stderr, "FAIL: protocol_build_register: '%s'\n", out);
        return 1;
    }
    printf("PASS: protocol_build_register\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_protocol_parse_type();
    failures += test_protocol_parse_request();
    failures += test_protocol_parse_intent();
    failures += test_protocol_build_response();
    failures += test_protocol_build_ping();
    failures += test_protocol_build_error();
    failures += test_protocol_build_register();

    if (failures) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll Protocol tests passed\n");
    return 0;
}
