#include "../include/common.h"
#include "../json/json.h"
#include <stdio.h>
#include <string.h>

static int test_json_extract_string(void) {
    const char *json = "{\"name\":\"desktop-agent\",\"version\":\"0.1.0\"}";
    char out[64];
    int ret = json_extract_string(json, "name", out, sizeof(out));
    if (ret != 0 || strcmp(out, "desktop-agent") != 0) {
        fprintf(stderr, "FAIL: json_extract_string: ret=%d out='%s'\n", ret, out);
        return 1;
    }
    printf("PASS: json_extract_string\n");
    return 0;
}

static int test_json_extract_int(void) {
    const char *json = "{\"id\":42,\"score\":99}";
    long long val;
    if (json_extract_int(json, "id", &val) != 0 || val != 42) {
        fprintf(stderr, "FAIL: json_extract_int: val=%lld\n", val);
        return 1;
    }
    printf("PASS: json_extract_int\n");
    return 0;
}

static int test_json_extract_double(void) {
    const char *json = "{\"confidence\":0.85}";
    double val;
    if (json_extract_double(json, "confidence", &val) != 0 || val < 0.84 || val > 0.86) {
        fprintf(stderr, "FAIL: json_extract_double: val=%f\n", val);
        return 1;
    }
    printf("PASS: json_extract_double\n");
    return 0;
}

static int test_json_extract_bool(void) {
    const char *json = "{\"enabled\":true,\"disabled\":false}";
    bool val;
    if (json_extract_bool(json, "enabled", &val) != 0 || val != true) {
        fprintf(stderr, "FAIL: json_extract_bool(true)\n");
        return 1;
    }
    if (json_extract_bool(json, "disabled", &val) != 0 || val != false) {
        fprintf(stderr, "FAIL: json_extract_bool(false)\n");
        return 1;
    }
    printf("PASS: json_extract_bool\n");
    return 0;
}

static int test_json_extract_string_array(void) {
    const char *json = "{\"capabilities\":[\"open_app\",\"install_pkg\",\"search\"]}";
    char out[8][MAX_NAME_LEN];
    int count;
    if (json_extract_string_array(json, "capabilities", out, 8, &count) != 0 || count != 3) {
        fprintf(stderr, "FAIL: json_extract_string_array: count=%d\n", count);
        return 1;
    }
    if (strcmp(out[0], "open_app") != 0 || strcmp(out[1], "install_pkg") != 0 || strcmp(out[2], "search") != 0) {
        fprintf(stderr, "FAIL: json_extract_string_array: values wrong\n");
        return 1;
    }
    printf("PASS: json_extract_string_array\n");
    return 0;
}

static int test_json_has_key(void) {
    const char *json = "{\"type\":\"ping\"}";
    if (!json_has_key(json, "type")) {
        fprintf(stderr, "FAIL: json_has_key(true)\n");
        return 1;
    }
    if (json_has_key(json, "nonexistent")) {
        fprintf(stderr, "FAIL: json_has_key(false)\n");
        return 1;
    }
    printf("PASS: json_has_key\n");
    return 0;
}

static int test_json_escape_unescape(void) {
    const char *src = "hello \"world\"\n";
    char escaped[64];
    char unescaped[64];

    json_escape(src, escaped, sizeof(escaped));
    json_unescape(escaped, unescaped, sizeof(unescaped));

    if (strcmp(src, unescaped) != 0) {
        fprintf(stderr, "FAIL: json_escape/unescape roundtrip\n");
        return 1;
    }
    printf("PASS: json_escape/unescape\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_json_extract_string();
    failures += test_json_extract_int();
    failures += test_json_extract_double();
    failures += test_json_extract_bool();
    failures += test_json_extract_string_array();
    failures += test_json_has_key();
    failures += test_json_escape_unescape();

    if (failures) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll JSON tests passed\n");
    return 0;
}
