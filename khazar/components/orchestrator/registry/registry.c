#include "registry.h"
#include "logger/logger.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MODULE "registry"

struct registry {
    agent_info_t agents[MAX_AGENTS];
    int count;
    pthread_mutex_t mutex;
};

registry_t *registry_init(void) {
    registry_t *reg = calloc(1, sizeof(registry_t));
    if (!reg) return NULL;

    pthread_mutex_init(&reg->mutex, NULL);
    reg->count = 0;
    log_info(MODULE, "registry initialized");
    return reg;
}

int registry_register(registry_t *reg, const agent_info_t *agent) {
    if (!reg || !agent) return -1;

    pthread_mutex_lock(&reg->mutex);

    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->agents[i].name, agent->name) == 0) {
            reg->agents[i] = *agent;
            reg->agents[i].alive = true;
            reg->agents[i].last_heartbeat = time(NULL);
            pthread_mutex_unlock(&reg->mutex);
            log_info(MODULE, "agent updated: %s", agent->name);
            return 0;
        }
    }

    if (reg->count >= MAX_AGENTS) {
        pthread_mutex_unlock(&reg->mutex);
        log_error(MODULE, "registry full, cannot register %s", agent->name);
        return -1;
    }

    reg->agents[reg->count] = *agent;
    reg->agents[reg->count].alive = true;
    reg->agents[reg->count].last_heartbeat = time(NULL);
    reg->count++;
    pthread_mutex_unlock(&reg->mutex);

    log_info(MODULE, "agent registered: %s (%s)", agent->name, agent->version);
    return 0;
}

int registry_unregister(registry_t *reg, const char *name) {
    if (!reg || !name) return -1;

    pthread_mutex_lock(&reg->mutex);

    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->agents[i].name, name) == 0) {
            reg->agents[i] = reg->agents[--reg->count];
            pthread_mutex_unlock(&reg->mutex);
            log_info(MODULE, "agent unregistered: %s", name);
            return 0;
        }
    }

    pthread_mutex_unlock(&reg->mutex);
    log_warn(MODULE, "agent not found: %s", name);
    return -1;
}

agent_info_t *registry_find_by_capability(registry_t *reg, const char *capability) {
    if (!reg || !capability) return NULL;

    pthread_mutex_lock(&reg->mutex);

    for (int i = 0; i < reg->count; i++) {
        if (!reg->agents[i].alive) continue;
        for (int j = 0; j < reg->agents[i].cap_count; j++) {
            if (strcmp(reg->agents[i].capabilities[j], capability) == 0) {
                agent_info_t *agent = &reg->agents[i];
                pthread_mutex_unlock(&reg->mutex);
                return agent;
            }
        }
    }

    pthread_mutex_unlock(&reg->mutex);
    return NULL;
}

agent_info_t *registry_find_by_name(registry_t *reg, const char *name) {
    if (!reg || !name) return NULL;

    pthread_mutex_lock(&reg->mutex);

    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->agents[i].name, name) == 0) {
            agent_info_t *agent = &reg->agents[i];
            pthread_mutex_unlock(&reg->mutex);
            return agent;
        }
    }

    pthread_mutex_unlock(&reg->mutex);
    return NULL;
}

void registry_cleanup(registry_t *reg) {
    if (!reg) return;
    pthread_mutex_destroy(&reg->mutex);
    free(reg);
    log_info(MODULE, "registry cleaned up");
}
