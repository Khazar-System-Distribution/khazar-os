#include "json.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

static const char *skip_whitespace(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

static const char *find_key(const char *json, const char *key) {
    if (!json || !key) return NULL;

    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return NULL;

    p += strlen(search);
    p = skip_whitespace(p);
    if (*p != ':') return NULL;
    p++;
    p = skip_whitespace(p);
    return p;
}

int json_extract_string(const char *json, const char *key, char *out, size_t out_len) {
    if (!json || !key || !out || out_len == 0) return -1;

    const char *p = find_key(json, key);
    if (!p) return -1;

    if (*p != '"') return -1;
    p++;

    return json_unescape(p, out, out_len);
}

int json_extract_int(const char *json, const char *key, long long *out) {
    if (!json || !key || !out) return -1;

    const char *p = find_key(json, key);
    if (!p) return -1;

    char *end;
    *out = strtoll(p, &end, 10);
    if (end == p) return -1;
    return 0;
}

int json_extract_double(const char *json, const char *key, double *out) {
    if (!json || !key || !out) return -1;

    const char *p = find_key(json, key);
    if (!p) return -1;

    char *end;
    *out = strtod(p, &end);
    if (end == p) return -1;
    return 0;
}

int json_extract_bool(const char *json, const char *key, bool *out) {
    if (!json || !key || !out) return -1;

    const char *p = find_key(json, key);
    if (!p) return -1;

    if (strncmp(p, "true", 4) == 0) {
        *out = true;
        return 0;
    }
    if (strncmp(p, "false", 5) == 0) {
        *out = false;
        return 0;
    }
    return -1;
}

int json_extract_string_array(const char *json, const char *key,
                              char out[][MAX_NAME_LEN], int max_count, int *count) {
    if (!json || !key || !out || !count) return -1;

    *count = 0;
    const char *p = find_key(json, key);
    if (!p) return -1;

    if (*p != '[') return -1;
    p++;

    while (*p && *p != ']' && *count < max_count) {
        p = skip_whitespace(p);
        while (*p && *p != '"' && *p != ']') p++;
        if (*p == '"') {
            p++;
            int i = 0;
            while (*p && *p != '"' && i < MAX_NAME_LEN - 1) {
                if (*p == '\\' && *(p + 1)) p++;
                out[*count][i++] = *p++;
            }
            out[*count][i] = '\0';
            (*count)++;
            if (*p == '"') p++;
        }
        p = skip_whitespace(p);
        if (*p == ',') p++;
    }
    return 0;
}

int json_has_key(const char *json, const char *key) {
    if (!json || !key) return 0;
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    return strstr(json, search) != NULL;
}

int json_build_object(char *buf, size_t buf_len, const char *fmt, ...) {
    if (!buf || buf_len == 0 || !fmt) return -1;

    va_list args;
    va_start(args, fmt);

    int written = vsnprintf(buf, buf_len, fmt, args);

    va_end(args);
    return (written < 0 || (size_t)written >= buf_len) ? -1 : 0;
}

int json_escape(const char *src, char *dst, size_t dst_len) {
    if (!src || !dst || dst_len == 0) return -1;

    size_t i = 0, j = 0;
    while (src[i] && j < dst_len - 2) {
        switch (src[i]) {
            case '"':  dst[j++] = '\\'; dst[j++] = '"';  break;
            case '\\': dst[j++] = '\\'; dst[j++] = '\\'; break;
            case '\n': dst[j++] = '\\'; dst[j++] = 'n';  break;
            case '\r': dst[j++] = '\\'; dst[j++] = 'r';  break;
            case '\t': dst[j++] = '\\'; dst[j++] = 't';  break;
            default:   dst[j++] = src[i]; break;
        }
        i++;
    }
    dst[j] = '\0';
    return 0;
}

int json_unescape(const char *src, char *dst, size_t dst_len) {
    if (!src || !dst || dst_len == 0) return -1;

    size_t i = 0, j = 0;
    while (src[i] && src[i] != '"' && j < dst_len - 1) {
        if (src[i] == '\\' && src[i + 1]) {
            i++;
            switch (src[i]) {
                case '"':  dst[j++] = '"';  break;
                case '\\': dst[j++] = '\\'; break;
                case 'n':  dst[j++] = '\n'; break;
                case 'r':  dst[j++] = '\r'; break;
                case 't':  dst[j++] = '\t'; break;
                default:   dst[j++] = src[i]; break;
            }
        } else {
            dst[j++] = src[i];
        }
        i++;
    }
    dst[j] = '\0';
    return 0;
}
