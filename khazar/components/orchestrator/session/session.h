#ifndef SESSION_H
#define SESSION_H

#include "../include/common.h"

#define MAX_HISTORY_ENTRIES 50
#define SESSION_TIMEOUT     300

typedef struct {
    char     query[MAX_MESSAGE_SIZE];
    char     response[MAX_MESSAGE_SIZE];
    time_t   timestamp;
} history_entry_t;

typedef struct {
    char     session_id[64];
    char     client_id[64];
    history_entry_t history[MAX_HISTORY_ENTRIES];
    int      history_count;
    time_t   last_active;
    bool     active;
} session_t;

typedef struct session_manager session_manager_t;

session_manager_t *session_init(int max_sessions, int timeout_secs);
session_t         *session_get_or_create(session_manager_t *sm, const char *client_id);
int                session_add_history(session_manager_t *sm, const char *session_id, const char *query, const char *response);
void               session_cleanup(session_manager_t *sm);

#endif
