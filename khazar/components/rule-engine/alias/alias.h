#ifndef ALIAS_H
#define ALIAS_H

#include "common.h"

int  alias_init(void);
int  alias_add(const char *keyword, const char *canonical, rule_source_t source);
int  alias_load_defaults(void);
int  alias_resolve(const char *query, match_result_t *result);
void alias_clear_rules(rule_source_t source);
void alias_cleanup(void);

#endif
