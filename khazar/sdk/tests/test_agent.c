#include "../include/common.h"
#include "../agent/agent.h"
#include <stdio.h>
#include <string.h>

static int test_agent_init(void) {
    const char *caps[] = {"open_app", "search"};
    agent_config_t config = {
        .name = "test-agent",
        .version = "0.1.0",
        .capabilities = {caps[0], caps[1]},
        .cap_count = 2,
        .orchestrator_socket = DEFAULT_ORCHESTRATOR_SOCKET,
        .task_handler = NULL,
        .handler_ctx = NULL,
    };

    agent_t *a = agent_init(&config);
    if (!a) {
        fprintf(stderr, "FAIL: agent_init returned NULL\n");
        return 1;
    }

    if (agent_get_state(a) != AGENT_STATE_UNREGISTERED) {
        fprintf(stderr, "FAIL: agent_get_state: expected UNREGISTERED\n");
        agent_cleanup(a);
        return 1;
    }

    if (strcmp(agent_get_name(a), "test-agent") != 0) {
        fprintf(stderr, "FAIL: agent_get_name: '%s'\n", agent_get_name(a));
        agent_cleanup(a);
        return 1;
    }

    agent_cleanup(a);
    printf("PASS: agent_init\n");
    return 0;
}

static int test_agent_init_null_name(void) {
    agent_config_t config = {
        .name = NULL,
        .version = "0.1.0",
        .cap_count = 0,
    };
    agent_t *a = agent_init(&config);
    if (a != NULL) {
        fprintf(stderr, "FAIL: agent_init with NULL name should return NULL\n");
        agent_cleanup(a);
        return 1;
    }
    printf("PASS: agent_init_null_name\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_agent_init();
    failures += test_agent_init_null_name();

    if (failures) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll Agent tests passed\n");
    return 0;
}
