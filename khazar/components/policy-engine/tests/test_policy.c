#include "../include/common.h"
#include "../policy/policy.h"
#include "../protocol/protocol.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int test_policy_init(void) {
    policy_store_t *ps = policy_init();
    if (!ps) return 1;
    policy_cleanup(ps);
    printf("PASS: policy_init\n");
    return 0;
}

static int test_policy_add_and_check(void) {
    policy_store_t *ps = policy_init();
    if (!ps) return 1;

    policy_rule_t rule = {
        .action = POLICY_ALLOW,
        .priority = 10,
        .match_type = POLICY_MATCH_EXACT,
    };
    strcpy(rule.capability, "open_application");
    strcpy(rule.agent_name, "*");

    policy_add_rule(ps, &rule);

    policy_request_t req = {
        .id = 1,
    };
    strcpy(req.agent_name, "desktop-agent");
    strcpy(req.capability, "open_application");
    strcpy(req.target, "firefox");

    policy_response_t resp;
    policy_check(ps, &req, &resp);

    if (resp.action != POLICY_ALLOW) {
        fprintf(stderr, "FAIL: expected ALLOW, got %s\n", policy_action_str(resp.action));
        policy_cleanup(ps);
        return 1;
    }
    printf("PASS: policy_add_and_check (allow)\n");

    policy_request_t req2 = {
        .id = 2,
    };
    strcpy(req2.agent_name, "unknown-agent");
    strcpy(req2.capability, "system_shutdown");
    strcpy(req2.target, "now");

    policy_check(ps, &req2, &resp);
    if (resp.action != POLICY_DENY) {
        fprintf(stderr, "FAIL: expected DENY, got %s\n", policy_action_str(resp.action));
        policy_cleanup(ps);
        return 1;
    }
    printf("PASS: policy_add_and_check (deny)\n");

    policy_cleanup(ps);
    return 0;
}

static int test_policy_priority(void) {
    policy_store_t *ps = policy_init();
    if (!ps) return 1;

    policy_rule_t allow_all = {
        .action = POLICY_ALLOW,
        .priority = 5,
    };
    strcpy(allow_all.capability, "*");
    strcpy(allow_all.agent_name, "*");

    policy_rule_t deny_shutdown = {
        .action = POLICY_DENY,
        .priority = 50,
    };
    strcpy(deny_shutdown.capability, "system_shutdown");
    strcpy(deny_shutdown.agent_name, "*");

    policy_add_rule(ps, &allow_all);
    policy_add_rule(ps, &deny_shutdown);

    policy_request_t req = {.id = 1};
    strcpy(req.agent_name, "power-agent");
    strcpy(req.capability, "system_shutdown");

    policy_response_t resp;
    policy_check(ps, &req, &resp);

    if (resp.action != POLICY_DENY) {
        fprintf(stderr, "FAIL: higher priority deny should win, got %s\n",
                policy_action_str(resp.action));
        policy_cleanup(ps);
        return 1;
    }
    printf("PASS: policy_priority (higher wins)\n");

    policy_cleanup(ps);
    return 0;
}

static int test_protocol_parse_check(void) {
    const char *json = "{\"type\":\"policy_check\",\"id\":99,\"agent\":\"desktop-agent\",\"capability\":\"open_application\",\"target\":\"firefox\"}";
    policy_request_t req;
    if (protocol_parse_policy_check(json, strlen(json), &req) != 0) {
        fprintf(stderr, "FAIL: protocol_parse_policy_check returned error\n");
        return 1;
    }
    if (req.id != 99) {
        fprintf(stderr, "FAIL: wrong id %llu\n", (unsigned long long)req.id);
        return 1;
    }
    if (strcmp(req.agent_name, "desktop-agent") != 0) {
        fprintf(stderr, "FAIL: wrong agent %s\n", req.agent_name);
        return 1;
    }
    if (strcmp(req.capability, "open_application") != 0) {
        fprintf(stderr, "FAIL: wrong cap %s\n", req.capability);
        return 1;
    }
    if (strcmp(req.target, "firefox") != 0) {
        fprintf(stderr, "FAIL: wrong target %s\n", req.target);
        return 1;
    }
    printf("PASS: protocol_parse_policy_check\n");
    return 0;
}

static int test_protocol_build_response(void) {
    policy_response_t resp = {
        .id = 42,
        .action = POLICY_ALLOW,
    };
    strcpy(resp.reason, "allowed by default");
    strcpy(resp.matched_rule, "cap=* agent=*");

    char out[512];
    protocol_build_response(&resp, out, sizeof(out));

    if (!strstr(out, "\"action\":\"allow\"") || !strstr(out, "\"id\":42")) {
        fprintf(stderr, "FAIL: protocol_build_response: %s\n", out);
        return 1;
    }
    printf("PASS: protocol_build_response\n");
    return 0;
}

static int test_protocol_build_deny(void) {
    policy_response_t resp = {
        .id = 7,
        .action = POLICY_DENY,
    };
    strcpy(resp.reason, "root required for shutdown");

    char out[512];
    protocol_build_response(&resp, out, sizeof(out));

    if (!strstr(out, "\"action\":\"deny\"")) {
        fprintf(stderr, "FAIL: protocol_build_response deny: %s\n", out);
        return 1;
    }
    printf("PASS: protocol_build_response_deny\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_policy_init();
    failures += test_policy_add_and_check();
    failures += test_policy_priority();
    failures += test_protocol_parse_check();
    failures += test_protocol_build_response();
    failures += test_protocol_build_deny();

    if (failures) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll Policy Engine tests passed\n");
    return 0;
}
