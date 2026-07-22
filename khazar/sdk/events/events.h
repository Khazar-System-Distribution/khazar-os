#ifndef AI_SDK_EVENTS_H
#define AI_SDK_EVENTS_H

#include "../include/common.h"

#define MAX_SUBSCRIBERS       32
#define MAX_EVENT_TYPE_LEN    64
#define MAX_EVENT_DATA_LEN    4096

typedef void (*event_handler_t)(const char *event_type, const char *data, void *ctx);

typedef struct {
    char             event_type[MAX_EVENT_TYPE_LEN];
    event_handler_t  handler;
    void            *ctx;
} subscriber_t;

typedef struct {
    subscriber_t subscribers[MAX_SUBSCRIBERS];
    int          count;
    bool         running;
    bool         has_pending;
    char         pending_type[MAX_EVENT_TYPE_LEN];
    char         pending_data[MAX_EVENT_DATA_LEN];
} event_loop_t;

event_loop_t *event_loop_init(void);
int           event_subscribe(event_loop_t *el, const char *event_type,
                              event_handler_t handler, void *ctx);
int           event_unsubscribe(event_loop_t *el, const char *event_type,
                                event_handler_t handler);
int           event_publish(event_loop_t *el, const char *event_type, const char *data);
int           event_process_pending(event_loop_t *el);
void          event_loop_cleanup(event_loop_t *el);

#endif
