#ifndef INTENT_CLIENT_H
#define INTENT_CLIENT_H

#include "../include/common.h"
#include "../router/router.h"

typedef struct intent_client intent_client_t;

intent_client_t *intent_client_init(const char *socket_path);
int              intent_client_classify(intent_client_t *ic, const char *query,
                                         intent_t *intent, float *confidence);
void             intent_client_cleanup(intent_client_t *ic);

#endif
