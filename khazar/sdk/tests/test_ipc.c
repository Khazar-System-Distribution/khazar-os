#include "../include/common.h"
#include "../ipc/ipc.h"
#include <stdio.h>
#include <string.h>

static int test_ipc_client_init(void) {
    ipc_client_t *client = ipc_client_init("/tmp/test-sdk.sock");
    if (!client) {
        fprintf(stderr, "FAIL: ipc_client_init returned NULL\n");
        return 1;
    }
    if (ipc_client_is_connected(client)) {
        fprintf(stderr, "FAIL: ipc_client_is_connected should be false\n");
        ipc_client_cleanup(client);
        return 1;
    }
    if (ipc_client_get_fd(client) != -1) {
        fprintf(stderr, "FAIL: ipc_client_get_fd should be -1\n");
        ipc_client_cleanup(client);
        return 1;
    }
    ipc_client_cleanup(client);
    printf("PASS: ipc_client_init\n");
    return 0;
}

static int test_ipc_server_init(void) {
    ipc_server_t *srv = ipc_server_init("/tmp/test-sdk-server.sock", 32);
    if (!srv) {
        fprintf(stderr, "FAIL: ipc_server_init returned NULL\n");
        return 1;
    }
    ipc_server_cleanup(srv);
    printf("PASS: ipc_server_init\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_ipc_client_init();
    failures += test_ipc_server_init();

    if (failures) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll IPC tests passed\n");
    return 0;
}
