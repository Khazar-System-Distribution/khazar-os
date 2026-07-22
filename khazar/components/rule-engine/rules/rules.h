#ifndef RULES_H
#define RULES_H

#include "../include/common.h"

int  rules_init(const char *rules_dir);
int  rules_load_all(void);
int  rules_load_regex(const char *path);
int  rules_load_tokens(const char *path);
int  rules_load_intents(const char *path);
int  rules_load_aliases(const char *path);
void rules_cleanup(void);

#endif
