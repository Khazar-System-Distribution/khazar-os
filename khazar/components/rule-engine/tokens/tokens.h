#ifndef TOKENS_H
#define TOKENS_H

#include "common.h"

int  tokens_init(void);
int  tokens_add(const char *token, intent_type_t intent, int weight, rule_source_t source);
int  tokens_load_defaults(void);
int  tokens_lookup(const char *query, match_result_t *result);
void tokens_clear_rules(rule_source_t source);
void tokens_cleanup(void);

#endif
