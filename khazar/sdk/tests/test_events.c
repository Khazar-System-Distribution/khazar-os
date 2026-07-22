#include "../include/common.h"
#include "../events/events.h"
#include <stdio.h>
#include <string.h>

static int received_count = 0;
static char last_data[256];

static void test_event_handler(const char *event_type, const char *data, void *ctx) {
    (void)ctx;
    (void)event_type;
    received_count++;
    strncpy(last_data, data, sizeof(last_data) - 1);
}

static int test_event_subscribe_publish(void) {
    event_loop_t *el = event_loop_init();
    if (!el) {
        fprintf(stderr, "FAIL: event_loop_init returned NULL\n");
        return 1;
    }

    received_count = 0;
    memset(last_data, 0, sizeof(last_data));

    if (event_subscribe(el, "task.completed", test_event_handler, NULL) != 0) {
        fprintf(stderr, "FAIL: event_subscribe\n");
        event_loop_cleanup(el);
        return 1;
    }

    int delivered = event_publish(el, "task.completed", "{\"result\":\"ok\"}");
    if (delivered != 1 || received_count != 1) {
        fprintf(stderr, "FAIL: event_publish delivered=%d recv=%d\n", delivered, received_count);
        event_loop_cleanup(el);
        return 1;
    }

    if (!strstr(last_data, "ok")) {
        fprintf(stderr, "FAIL: event_publish data: '%s'\n", last_data);
        event_loop_cleanup(el);
        return 1;
    }

    printf("PASS: event_subscribe_publish\n");
    event_loop_cleanup(el);
    return 0;
}

static int test_event_unsubscribe(void) {
    event_loop_t *el = event_loop_init();
    if (!el) return 1;

    received_count = 0;
    event_subscribe(el, "test.event", test_event_handler, NULL);
    event_unsubscribe(el, "test.event", test_event_handler);

    int delivered = event_publish(el, "test.event", "data");
    if (delivered != 0 || received_count != 0) {
        fprintf(stderr, "FAIL: event_unsubscribe: still delivered %d\n", delivered);
        event_loop_cleanup(el);
        return 1;
    }

    printf("PASS: event_unsubscribe\n");
    event_loop_cleanup(el);
    return 0;
}

static int test_event_multiple_subscribers(void) {
    event_loop_t *el = event_loop_init();
    if (!el) return 1;

    received_count = 0;
    event_subscribe(el, "multi.event", test_event_handler, NULL);
    event_subscribe(el, "other.event", test_event_handler, NULL);

    int delivered = event_publish(el, "multi.event", "test");
    if (delivered != 1 || received_count != 1) {
        fprintf(stderr, "FAIL: event_multiple_subscribers: delivered=%d recv=%d\n",
                delivered, received_count);
        event_loop_cleanup(el);
        return 1;
    }

    printf("PASS: event_multiple_subscribers\n");
    event_loop_cleanup(el);
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_event_subscribe_publish();
    failures += test_event_unsubscribe();
    failures += test_event_multiple_subscribers();

    if (failures) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll Events tests passed\n");
    return 0;
}
