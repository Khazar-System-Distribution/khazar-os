#ifndef INTENT_H
#define INTENT_H

#include "common.h"

int  intent_init(void);
int  intent_add(const char *query, intent_type_t type, const char *target, const char *action, rule_source_t source);
int  intent_load_defaults(void);
int  intent_lookup(const char *query, match_result_t *result);
void intent_clear_rules(rule_source_t source);
void intent_cleanup(void);

#endif
