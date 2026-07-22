#ifndef COMMON_H
#define COMMON_H

#include "../../../sdk/include/common.h"
#include <string.h>

#define SOCKET_PATH_DEFAULT     "/run/ai-policy-engine.sock"
#define MAX_POLICIES            512
#define MAX_POLICY_PATH_LEN     512
#define POLICY_DEFAULT_DIR      "/etc/ai-policy-engine/rules"

typedef enum {
    POLICY_ALLOW,
    POLICY_DENY,
    POLICY_ASK_USER
} policy_action_t;

typedef enum {
    POLICY_MATCH_EXACT,
    POLICY_MATCH_PATTERN,
    POLICY_MATCH_ALL
} policy_match_t;

typedef struct {
    char             capability[64];
    char             agent_name[MAX_NAME_LEN];
    policy_action_t  action;
    bool             require_root;
    policy_match_t   match_type;
    char             message[256];
    int              priority;
} policy_rule_t;

typedef struct {
    uint64_t    id;
    char        agent_name[MAX_NAME_LEN];
    char        capability[64];
    char        target[128];
} policy_request_t;

typedef struct {
    uint64_t         id;
    policy_action_t  action;
    char             reason[256];
    char             matched_rule[64];
} policy_response_t;

static inline const char *policy_action_str(policy_action_t a) {
    switch (a) {
        case POLICY_ALLOW:    return "allow";
        case POLICY_DENY:     return "deny";
        case POLICY_ASK_USER: return "ask_user";
        default:              return "unknown";
    }
}

static inline policy_action_t policy_action_from_str(const char *s) {
    if (!s) return POLICY_DENY;
    if (strcmp(s, "allow") == 0)     return POLICY_ALLOW;
    if (strcmp(s, "deny") == 0)      return POLICY_DENY;
    if (strcmp(s, "ask_user") == 0)  return POLICY_ASK_USER;
    return POLICY_DENY;
}

#endif
