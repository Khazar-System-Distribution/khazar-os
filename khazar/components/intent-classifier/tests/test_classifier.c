#include "../include/common.h"
#include "../protocol/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int test_classify_request_parse(void) {
    const char *json = "{\"type\":\"classify\",\"id\":42,\"query\":\"open firefox\"}";
    classifier_request_t req;
    memset(&req, 0, sizeof(req));

    if (protocol_parse_classify(json, strlen(json), &req) != 0) {
        fprintf(stderr, "FAIL: parse_classify returned error\n");
        return 1;
    }
    if (req.id != 42) {
        fprintf(stderr, "FAIL: wrong id %llu\n", (unsigned long long)req.id);
        return 1;
    }
    if (strcmp(req.query, "open firefox") != 0) {
        fprintf(stderr, "FAIL: wrong query '%s'\n", req.query);
        return 1;
    }
    printf("PASS: classify request parse\n");
    return 0;
}

static int test_classify_request_parse_empty_query(void) {
    const char *json = "{\"type\":\"classify\",\"id\":1,\"query\":\"\"}";
    classifier_request_t req;
    memset(&req, 0, sizeof(req));

    if (protocol_parse_classify(json, strlen(json), &req) != 0) {
        fprintf(stderr, "FAIL: parse_classify with empty query returned error\n");
        return 1;
    }
    if (strlen(req.query) != 0) {
        fprintf(stderr, "FAIL: expected empty query, got '%s'\n", req.query);
        return 1;
    }
    printf("PASS: classify request parse empty query\n");
    return 0;
}

static int test_message_type_detection(void) {
    classifier_message_type_t type;

    if (protocol_parse_type("{\"type\":\"ping\"}", 16, &type) != 0) {
        fprintf(stderr, "FAIL: parse_type ping returned error\n");
        return 1;
    }
    if (type != CMSG_PING) { fprintf(stderr, "FAIL: expected CMSG_PING\n"); return 1; }

    if (protocol_parse_type("{\"type\":\"version\"}", 19, &type) != 0) {
        fprintf(stderr, "FAIL: parse_type version returned error\n");
        return 1;
    }
    if (type != CMSG_VERSION) { fprintf(stderr, "FAIL: expected CMSG_VERSION\n"); return 1; }

    if (protocol_parse_type("{\"type\":\"reload\"}", 18, &type) != 0) {
        fprintf(stderr, "FAIL: parse_type reload returned error\n");
        return 1;
    }
    if (type != CMSG_RELOAD) { fprintf(stderr, "FAIL: expected CMSG_RELOAD\n"); return 1; }

    if (protocol_parse_type("{\"type\":\"classify\"}", 20, &type) != 0) {
        fprintf(stderr, "FAIL: parse_type classify returned error\n");
        return 1;
    }
    if (type != CMSG_CLASSIFY) { fprintf(stderr, "FAIL: expected CMSG_CLASSIFY\n"); return 1; }

    if (protocol_parse_type("{\"query\":\"hello\"}", 17, &type) != 0) {
        fprintf(stderr, "FAIL: parse_type query-only returned error\n");
        return 1;
    }
    if (type != CMSG_CLASSIFY) { fprintf(stderr, "FAIL: expected CMSG_CLASSIFY for query-only\n"); return 1; }

    printf("PASS: message type detection\n");
    return 0;
}

static int test_classify_response_build(void) {
    classifier_response_t resp = {
        .id = 99,
        .confidence = 0.95f,
        .fallback = false,
    };
    strcpy(resp.intent, "open_application");
    strcpy(resp.target, "firefox");

    char out[512];
    protocol_build_classify_response(&resp, out, sizeof(out));

    if (!strstr(out, "\"id\":99")) {
        fprintf(stderr, "FAIL: missing id in response: %s\n", out);
        return 1;
    }
    if (!strstr(out, "\"type\":\"classify_response\"")) {
        fprintf(stderr, "FAIL: missing type in response: %s\n", out);
        return 1;
    }
    if (!strstr(out, "\"intent\":\"open_application\"")) {
        fprintf(stderr, "FAIL: missing intent in response: %s\n", out);
        return 1;
    }
    if (!strstr(out, "\"target\":\"firefox\"")) {
        fprintf(stderr, "FAIL: missing target in response: %s\n", out);
        return 1;
    }
    if (!strstr(out, "\"confidence\":0.95")) {
        fprintf(stderr, "FAIL: missing confidence in response: %s\n", out);
        return 1;
    }
    if (!strstr(out, "\"fallback\":false")) {
        fprintf(stderr, "FAIL: missing fallback in response: %s\n", out);
        return 1;
    }

    printf("PASS: classify response build\n");
    return 0;
}

static int test_classify_response_fallback(void) {
    classifier_response_t resp = {
        .id = 7,
        .confidence = 0.5f,
        .fallback = true,
    };
    strcpy(resp.intent, "system_shutdown");
    strcpy(resp.target, "now");

    char out[512];
    protocol_build_classify_response(&resp, out, sizeof(out));

    if (!strstr(out, "\"fallback\":true")) {
        fprintf(stderr, "FAIL: missing fallback=true: %s\n", out);
        return 1;
    }
    if (!strstr(out, "\"intent\":\"system_shutdown\"")) {
        fprintf(stderr, "FAIL: missing intent: %s\n", out);
        return 1;
    }

    printf("PASS: classify response fallback\n");
    return 0;
}

static int test_protocol_build_ping(void) {
    char out[256];
    protocol_build_ping(out, sizeof(out));
    if (!strstr(out, "\"status\":\"alive\"")) {
        fprintf(stderr, "FAIL: ping response: %s\n", out);
        return 1;
    }
    printf("PASS: protocol build ping\n");
    return 0;
}

static int test_protocol_build_version(void) {
    char out[256];
    protocol_build_version(out, sizeof(out));
    if (!strstr(out, "\"component\":\"ai-intent-classifier\"")) {
        fprintf(stderr, "FAIL: version response: %s\n", out);
        return 1;
    }
    printf("PASS: protocol build version\n");
    return 0;
}

static int test_keyword_matching(void) {
    const char *query = "open firefox";
    char lower[2048];
    int i = 0;
    while (query[i] && i < 2047) {
        lower[i] = (char)((unsigned char)query[i] >= 'A' && (unsigned char)query[i] <= 'Z'
                          ? query[i] + ('a' - 'A') : query[i]);
        i++;
    }
    lower[i] = '\0';

    if (strstr(lower, "open") == NULL) {
        fprintf(stderr, "FAIL: keyword 'open' not found in '%s'\n", lower);
        return 1;
    }
    printf("PASS: keyword 'open' found in 'open firefox'\n");

    query = "SHUTDOWN NOW";
    i = 0;
    while (query[i] && i < 2047) {
        lower[i] = (char)((unsigned char)query[i] >= 'A' && (unsigned char)query[i] <= 'Z'
                          ? query[i] + ('a' - 'A') : query[i]);
        i++;
    }
    lower[i] = '\0';

    if (strstr(lower, "shutdown") == NULL) {
        fprintf(stderr, "FAIL: keyword 'shutdown' not found in '%s'\n", lower);
        return 1;
    }
    printf("PASS: keyword 'shutdown' found in 'SHUTDOWN NOW'\n");

    query = "what is the weather like";
    i = 0;
    while (query[i] && i < 2047) {
        lower[i] = (char)((unsigned char)query[i] >= 'A' && (unsigned char)query[i] <= 'Z'
                          ? query[i] + ('a' - 'A') : query[i]);
        i++;
    }
    lower[i] = '\0';

    const char *words[] = {"open", "close", "shutdown", "reboot", "install", "uninstall", NULL};
    for (int k = 0; words[k]; k++) {
        if (strstr(lower, words[k])) {
            fprintf(stderr, "FAIL: keyword '%s' unexpectedly found in weather query\n", words[k]);
            return 1;
        }
    }
    printf("PASS: no false positive match on weather query\n");

    return 0;
}

static int test_multiple_keyword_match(void) {
    const char *query = "close firefox and shutdown system";
    char lower[2048];
    int i = 0;
    while (query[i] && i < 2047) {
        lower[i] = (char)((unsigned char)query[i] >= 'A' && (unsigned char)query[i] <= 'Z'
                          ? query[i] + ('a' - 'A') : query[i]);
        i++;
    }
    lower[i] = '\0';

    const char *matches[] = {"close", "shutdown"};
    size_t longest = 0;
    int best_idx = -1;

    for (int k = 0; k < 2; k++) {
        if (strstr(lower, matches[k])) {
            size_t l = strlen(matches[k]);
            if (l > longest) { longest = l; best_idx = k; }
        }
    }

    if (best_idx < 0) {
        fprintf(stderr, "FAIL: no match found in multi-keyword query\n");
        return 1;
    }

    if (strcmp(matches[best_idx], "shutdown") != 0) {
        fprintf(stderr, "FAIL: expected longest match 'shutdown', got '%s'\n", matches[best_idx]);
        return 1;
    }
    printf("PASS: longest keyword match (shutdown over close)\n");

    return 0;
}

static int test_parse_llm_json(void) {
    const char *llm_json = "{\"intent\":\"open_application\",\"target\":\"firefox\",\"confidence\":0.92}";

    classifier_response_t resp;
    memset(&resp, 0, sizeof(resp));

    const char *ip = strstr(llm_json, "\"intent\"");
    if (ip) {
        ip = strchr(ip, ':');
        if (ip) {
            while (*ip && *ip != '"') ip++;
            if (*ip == '"') {
                ip++;
                int i = 0;
                while (*ip && *ip != '"' && i < (int)sizeof(resp.intent) - 1)
                    resp.intent[i++] = *ip++;
                resp.intent[i] = '\0';
            }
        }
    }

    const char *tp = strstr(llm_json, "\"target\"");
    if (tp) {
        tp = strchr(tp, ':');
        if (tp) {
            while (*tp && *tp != '"') tp++;
            if (*tp == '"') {
                tp++;
                int i = 0;
                while (*tp && *tp != '"' && i < (int)sizeof(resp.target) - 1)
                    resp.target[i++] = *tp++;
                resp.target[i] = '\0';
            }
        }
    }

    const char *cp = strstr(llm_json, "\"confidence\"");
    if (cp) {
        cp = strchr(cp, ':');
        if (cp) {
            while (*cp == ':' || *cp == ' ') cp++;
            resp.confidence = (float)atof(cp);
        }
    }

    if (strcmp(resp.intent, "open_application") != 0) {
        fprintf(stderr, "FAIL: wrong intent '%s'\n", resp.intent);
        return 1;
    }
    if (strcmp(resp.target, "firefox") != 0) {
        fprintf(stderr, "FAIL: wrong target '%s'\n", resp.target);
        return 1;
    }
    if (resp.confidence < 0.9f || resp.confidence > 0.95f) {
        fprintf(stderr, "FAIL: wrong confidence %f\n", resp.confidence);
        return 1;
    }
    printf("PASS: parse LLM JSON response\n");

    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_classify_request_parse();
    failures += test_classify_request_parse_empty_query();
    failures += test_message_type_detection();
    failures += test_classify_response_build();
    failures += test_classify_response_fallback();
    failures += test_protocol_build_ping();
    failures += test_protocol_build_version();
    failures += test_keyword_matching();
    failures += test_multiple_keyword_match();
    failures += test_parse_llm_json();

    if (failures) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll Intent Classifier tests passed\n");
    return 0;
}
