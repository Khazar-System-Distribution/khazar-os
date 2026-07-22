#include "session.h"
#include "logger/logger.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#define MODULE "session"

struct session_manager {
    session_t sessions[MAX_SESSIONS];
    int count;
    int max_sessions;
    int timeout_secs;
    pthread_mutex_t mutex;
};

session_manager_t *session_init(int max_sessions, int timeout_secs) {
    session_manager_t *sm = calloc(1, sizeof(session_manager_t));
    if (!sm) return NULL;

    sm->max_sessions = max_sessions > 0 ? max_sessions : MAX_SESSIONS;
    sm->timeout_secs = timeout_secs > 0 ? timeout_secs : SESSION_TIMEOUT;
    sm->count = 0;
    pthread_mutex_init(&sm->mutex, NULL);

    log_info(MODULE, "session manager initialized (max=%d, timeout=%ds)", sm->max_sessions, sm->timeout_secs);
    return sm;
}

static void generate_session_id(const char *client_id, char *out, size_t len) {
    snprintf(out, len, "%s_%ld", client_id, time(NULL));
}

session_t *session_get_or_create(session_manager_t *sm, const char *client_id) {
    if (!sm || !client_id) return NULL;

    pthread_mutex_lock(&sm->mutex);
    time_t now = time(NULL);

    for (int i = 0; i < sm->count; i++) {
        if (strcmp(sm->sessions[i].client_id, client_id) == 0) {
            if (now - sm->sessions[i].last_active < sm->timeout_secs) {
                sm->sessions[i].last_active = now;
                session_t *s = &sm->sessions[i];
                pthread_mutex_unlock(&sm->mutex);
                return s;
            }
            sm->sessions[i].active = false;
        }
    }

    for (int i = 0; i < sm->count; i++) {
        if (!sm->sessions[i].active) {
            memset(&sm->sessions[i], 0, sizeof(session_t));
            strncpy(sm->sessions[i].client_id, client_id, sizeof(sm->sessions[i].client_id) - 1);
            generate_session_id(client_id, sm->sessions[i].session_id, sizeof(sm->sessions[i].session_id));
            sm->sessions[i].last_active = now;
            sm->sessions[i].active = true;
            session_t *s = &sm->sessions[i];
            pthread_mutex_unlock(&sm->mutex);
            return s;
        }
    }

    if (sm->count < sm->max_sessions) {
        int idx = sm->count++;
        strncpy(sm->sessions[idx].client_id, client_id, sizeof(sm->sessions[idx].client_id) - 1);
        generate_session_id(client_id, sm->sessions[idx].session_id, sizeof(sm->sessions[idx].session_id));
        sm->sessions[idx].last_active = now;
        sm->sessions[idx].active = true;
        session_t *s = &sm->sessions[idx];
        pthread_mutex_unlock(&sm->mutex);
        return s;
    }

    pthread_mutex_unlock(&sm->mutex);
    log_warn(MODULE, "max sessions reached, cannot create for %s", client_id);
    return NULL;
}

int session_add_history(session_manager_t *sm, const char *session_id, const char *query, const char *response) {
    if (!sm || !session_id) return -1;

    pthread_mutex_lock(&sm->mutex);

    for (int i = 0; i < sm->count; i++) {
        if (strcmp(sm->sessions[i].session_id, session_id) == 0) {
            session_t *s = &sm->sessions[i];
            if (s->history_count < MAX_HISTORY_ENTRIES) {
                int h = s->history_count++;
                strncpy(s->history[h].query, query ? query : "", sizeof(s->history[h].query) - 1);
                strncpy(s->history[h].response, response ? response : "", sizeof(s->history[h].response) - 1);
                s->history[h].timestamp = time(NULL);
            }
            s->last_active = time(NULL);
            pthread_mutex_unlock(&sm->mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&sm->mutex);
    return -1;
}

void session_cleanup(session_manager_t *sm) {
    if (!sm) return;
    pthread_mutex_destroy(&sm->mutex);
    free(sm);
    log_info(MODULE, "session manager cleaned up");
}
