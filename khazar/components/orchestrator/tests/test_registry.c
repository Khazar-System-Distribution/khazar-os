#include "../registry/registry.h"
#include "logger/logger.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    logger_init(LOG_TRACE);
    logger_set_level(LOG_WARN);

    registry_t *reg = registry_init();
    assert(reg != NULL);

    agent_info_t agent;
    memset(&agent, 0, sizeof(agent));
    strcpy(agent.name, "test-agent");
    strcpy(agent.version, "1.0");
    strcpy(agent.capabilities[0], "test_cap");
    agent.cap_count = 1;
    agent.alive = true;

    assert(registry_register(reg, &agent) == 0);
    fprintf(stderr, "registry: agent registered OK\n");

    agent_info_t *found = registry_find_by_capability(reg, "test_cap");
    assert(found != NULL);
    assert(strcmp(found->name, "test-agent") == 0);
    fprintf(stderr, "registry: found by capability OK\n");

    found = registry_find_by_name(reg, "test-agent");
    assert(found != NULL);
    fprintf(stderr, "registry: found by name OK\n");

    assert(registry_unregister(reg, "test-agent") == 0);
    fprintf(stderr, "registry: unregistered OK\n");

    assert(registry_find_by_name(reg, "test-agent") == NULL);
    fprintf(stderr, "registry: confirmed removal OK\n");

    registry_cleanup(reg);
    logger_cleanup();
    fprintf(stderr, "registry: ALL TESTS PASSED\n");
    return 0;
}
