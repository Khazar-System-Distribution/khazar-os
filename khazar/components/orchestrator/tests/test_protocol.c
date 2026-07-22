#include "../protocol/protocol.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    char response[512];
    message_type_t type;
    request_t req;

    assert(protocol_parse("{\"type\":\"ping\"}", 16, &type, &req) == 0);
    assert(type == MSG_PING);
    fprintf(stderr, "protocol: PING parse OK\n");

    assert(protocol_parse("{\"type\":\"request\",\"query\":\"firefox ac\"}", 42, &type, &req) == 0);
    assert(type == MSG_REQUEST);
    assert(strcmp(req.query, "firefox ac") == 0);
    fprintf(stderr, "protocol: REQUEST parse OK (query=%s)\n", req.query);

    protocol_build_ping_response(response, sizeof(response));
    assert(strstr(response, "\"status\":\"alive\"") != NULL);
    fprintf(stderr, "protocol: PING response OK (%s)\n", response);

    assert(protocol_parse("{\"type\":\"register\",\"name\":\"test\"}", 37, &type, &req) == 0);
    assert(type == MSG_REGISTER);
    fprintf(stderr, "protocol: REGISTER parse OK\n");

    fprintf(stderr, "protocol: ALL TESTS PASSED\n");
    return 0;
}
