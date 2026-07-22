#include "router.h"
#include "logger/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define MODULE "router"
#define MAX_KEYWORDS 32

typedef struct {
    char keyword[64];
    char capability[64];
} keyword_entry_t;

struct router {
    registry_t *registry;
    keyword_entry_t keywords[MAX_KEYWORDS];
    int keyword_count;
    pthread_mutex_t mutex;
};

router_t *router_init(registry_t *reg) {
    router_t *rt = calloc(1, sizeof(router_t));
    if (!rt) return NULL;

    rt->registry = reg;
    rt->keyword_count = 0;
    pthread_mutex_init(&rt->mutex, NULL);

    const char *pairs[][2] = {
        {"ac", "open_application"},
        {"aç", "open_application"},
        {"bagla", "close_application"},
        {"bağla", "close_application"},
        {"qur", "install_package"},
        {"kur", "install_package"},
        {"install", "install_package"},
        {"sil", "remove_package"},
        {"remove", "remove_package"},
        {"ara", "search_package"},
        {"search", "search_package"},
        {"wifi", "network_management"},
    };

    for (size_t i = 0; i < sizeof(pairs)/sizeof(pairs[0]) && rt->keyword_count < MAX_KEYWORDS; i++) {
        keyword_entry_t *k = &rt->keywords[rt->keyword_count++];
        snprintf(k->keyword, sizeof(k->keyword), "%s", pairs[i][0]);
        snprintf(k->capability, sizeof(k->capability), "%s", pairs[i][1]);
    }

    log_info(MODULE, "router initialized with %d keywords", rt->keyword_count);
    return rt;
}

int router_resolve(router_t *rt, const request_t *req, intent_t *intent) {
    if (!rt || !req || !intent) return -1;

    memset(intent, 0, sizeof(*intent));

    char query_lower[MAX_MESSAGE_SIZE];
    int idx = 0;
    for (const char *p = req->query; *p && idx < (int)sizeof(query_lower) - 1; p++) {
        if (*p >= 'A' && *p <= 'Z')
            query_lower[idx++] = *p + 32;
        else
            query_lower[idx++] = *p;
    }
    query_lower[idx] = '\0';

    pthread_mutex_lock(&rt->mutex);

    for (int i = 0; i < rt->keyword_count; i++) {
        if (strstr(query_lower, rt->keywords[i].keyword)) {
            snprintf(intent->required_capability, sizeof(intent->required_capability),
                     "%s", rt->keywords[i].capability);
            snprintf(intent->target, sizeof(intent->target), "%s", query_lower);
            pthread_mutex_unlock(&rt->mutex);
            log_debug(MODULE, "matched keyword '%s' -> capability %s",
                      rt->keywords[i].keyword, rt->keywords[i].capability);
            return 0;
        }
    }

    pthread_mutex_unlock(&rt->mutex);
    log_debug(MODULE, "no keyword match for: %s", req->query);
    return -1;
}

void router_cleanup(router_t *rt) {
    if (!rt) return;
    pthread_mutex_destroy(&rt->mutex);
    free(rt);
    log_info(MODULE, "router cleaned up");
}
