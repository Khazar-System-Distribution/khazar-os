#ifndef AI_SDK_JSON_H
#define AI_SDK_JSON_H

#include "../include/common.h"

int json_extract_string(const char *json, const char *key, char *out, size_t out_len);

int json_extract_int(const char *json, const char *key, long long *out);

int json_extract_double(const char *json, const char *key, double *out);

int json_extract_bool(const char *json, const char *key, bool *out);

int json_extract_string_array(const char *json, const char *key,
                              char out[][MAX_NAME_LEN], int max_count, int *count);

int json_has_key(const char *json, const char *key);

int json_build_object(char *buf, size_t buf_len, const char *fmt, ...);

int json_escape(const char *src, char *dst, size_t dst_len);

int json_unescape(const char *src, char *dst, size_t dst_len);

#endif
