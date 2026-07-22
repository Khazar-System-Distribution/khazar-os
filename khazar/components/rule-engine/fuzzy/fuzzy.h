#ifndef FUZZY_H
#define FUZZY_H

#include "common.h"

int  fuzzy_init(void);
int  fuzzy_set_threshold(float threshold);
int  fuzzy_match(const char *query, match_result_t *result);
void fuzzy_cleanup(void);

#endif
