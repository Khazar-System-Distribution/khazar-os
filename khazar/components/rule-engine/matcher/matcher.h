#ifndef MATCHER_H
#define MATCHER_H

#include "common.h"

int  matcher_init(void);
int  matcher_add_rule(const char *pattern, intent_type_t intent, int priority, rule_source_t source);
int  matcher_load_defaults(void);
int  matcher_match(const char *query, match_result_t *result);
void matcher_clear_rules(rule_source_t source);
void matcher_cleanup(void);

#endif
