#include "events.h"
#include "../logger/logger.h"
#include <stdlib.h>
#include <string.h>

#define MODULE "events"

event_loop_t *event_loop_init(void) {
    event_loop_t *el = calloc(1, sizeof(event_loop_t));
    if (!el) return NULL;

    el->count = 0;
    el->running = false;
    el->has_pending = false;

    log_info(MODULE, "event loop initialised");
    return el;
}

int event_subscribe(event_loop_t *el, const char *event_type,
                    event_handler_t handler, void *ctx) {
    if (!el || !event_type || !handler) return -1;
    if (el->count >= MAX_SUBSCRIBERS) {
        log_error(MODULE, "max subscribers reached");
        return -1;
    }

    for (int i = 0; i < el->count; i++) {
        if (strcmp(el->subscribers[i].event_type, event_type) == 0 &&
            el->subscribers[i].handler == handler) {
            el->subscribers[i].ctx = ctx;
            log_debug(MODULE, "subscriber updated: %s", event_type);
            return 0;
        }
    }

    subscriber_t *s = &el->subscribers[el->count++];
    strncpy(s->event_type, event_type, sizeof(s->event_type) - 1);
    s->handler = handler;
    s->ctx = ctx;

    log_info(MODULE, "subscribed to '%s'", event_type);
    return 0;
}

int event_unsubscribe(event_loop_t *el, const char *event_type,
                      event_handler_t handler) {
    if (!el || !event_type || !handler) return -1;

    for (int i = 0; i < el->count; i++) {
        if (strcmp(el->subscribers[i].event_type, event_type) == 0 &&
            el->subscribers[i].handler == handler) {
            el->subscribers[i] = el->subscribers[--el->count];
            log_info(MODULE, "unsubscribed from '%s'", event_type);
            return 0;
        }
    }

    return -1;
}

int event_publish(event_loop_t *el, const char *event_type, const char *data) {
    if (!el || !event_type) return -1;

    int delivered = 0;
    for (int i = 0; i < el->count; i++) {
        if (strcmp(el->subscribers[i].event_type, event_type) == 0) {
            if (el->subscribers[i].handler) {
                el->subscribers[i].handler(event_type,
                    data ? data : "", el->subscribers[i].ctx);
                delivered++;
            }
        }
    }

    log_debug(MODULE, "published '%s' to %d subscribers", event_type, delivered);
    return delivered;
}

int event_process_pending(event_loop_t *el) {
    if (!el) return -1;

    if (el->has_pending) {
        int delivered = event_publish(el, el->pending_type, el->pending_data);
        el->has_pending = false;
        return delivered;
    }
    return 0;
}

void event_loop_cleanup(event_loop_t *el) {
    if (!el) return;
    free(el);
    log_info(MODULE, "event loop cleaned up");
}
