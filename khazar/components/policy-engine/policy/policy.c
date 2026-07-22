#include "policy.h"
#include "logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fnmatch.h>

#define MODULE "policy"

struct policy_store {
    policy_rule_t rules[MAX_POLICIES];
    int           count;
    pthread_mutex_t mutex;
};

static int match_agent(const char *rule_pattern, const char *agent_name) {
    if (!rule_pattern || !agent_name) return 0;
    if (strcmp(rule_pattern, "*") == 0) return 1;
    if (strcmp(rule_pattern, agent_name) == 0) return 1;
    if (fnmatch(rule_pattern, agent_name, 0) == 0) return 1;
    return 0;
}

static int match_capability(const char *rule_cap, const char *capability) {
    if (!rule_cap || !capability) return 0;
    if (strcmp(rule_cap, "*") == 0) return 1;
    return strcmp(rule_cap, capability) == 0;
}

policy_store_t *policy_init(void) {
    policy_store_t *ps = calloc(1, sizeof(policy_store_t));
    if (!ps) return NULL;

    ps->count = 0;
    pthread_mutex_init(&ps->mutex, NULL);

    log_info(MODULE, "policy store initialised");
    return ps;
}

int policy_load_file(policy_store_t *ps, const char *path) {
    if (!ps || !path) return -1;

    FILE *f = fopen(path, "r");
    if (!f) {
        log_warn(MODULE, "cannot open policy file: %s", path);
        return -1;
    }

    int loaded = 0;
    policy_rule_t rule;
    char line[512];
    bool in_rule = false;

    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        if (line[0] == '#' || line[0] == '\0') continue;

        if (strstr(line, "[rule.")) {
            if (in_rule) {
                pthread_mutex_lock(&ps->mutex);
                if (ps->count < MAX_POLICIES) {
                    ps->rules[ps->count++] = rule;
                    loaded++;
                }
                pthread_mutex_unlock(&ps->mutex);
            }
            memset(&rule, 0, sizeof(rule));
            rule.action = POLICY_ALLOW;
            rule.priority = 10;
            rule.match_type = POLICY_MATCH_EXACT;
            in_rule = true;
            continue;
        }

        if (!in_rule) continue;

        char key[128] = {0}, value[384] = {0};
        if (sscanf(line, "%127[^=]=%383[^\n]", key, value) != 2) continue;

        char *k = key;
        while (*k == ' ') k++;
        char *ke = k + strlen(k) - 1;
        while (ke > k && *ke == ' ') { *ke = '\0'; ke--; }

        char *v = value;
        while (*v == ' ') v++;
        size_t vl = strlen(v);
        if (vl >= 2 && v[0] == '"' && v[vl - 1] == '"') {
            v[vl - 1] = '\0';
            v++;
        }

        if (strcmp(k, "capability") == 0)
            strncpy(rule.capability, v, sizeof(rule.capability) - 1);
        else if (strcmp(k, "agent") == 0)
            strncpy(rule.agent_name, v, sizeof(rule.agent_name) - 1);
        else if (strcmp(k, "action") == 0)
            rule.action = policy_action_from_str(v);
        else if (strcmp(k, "require_root") == 0)
            rule.require_root = (strcmp(v, "true") == 0);
        else if (strcmp(k, "match_type") == 0) {
            if (strcmp(v, "pattern") == 0)      rule.match_type = POLICY_MATCH_PATTERN;
            else if (strcmp(v, "all") == 0)     rule.match_type = POLICY_MATCH_ALL;
            else                                 rule.match_type = POLICY_MATCH_EXACT;
        }
        else if (strcmp(k, "message") == 0)
            strncpy(rule.message, v, sizeof(rule.message) - 1);
        else if (strcmp(k, "priority") == 0)
            rule.priority = atoi(v);
    }

    if (in_rule) {
        pthread_mutex_lock(&ps->mutex);
        if (ps->count < MAX_POLICIES) {
            ps->rules[ps->count++] = rule;
            loaded++;
        }
        pthread_mutex_unlock(&ps->mutex);
    }

    fclose(f);
    log_info(MODULE, "loaded %d rules from %s", loaded, path);
    return loaded;
}

int policy_load_dir(policy_store_t *ps, const char *dir) {
    if (!ps || !dir) return -1;

    log_info(MODULE, "loading policies from %s", dir);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "ls %s/*.toml 2>/dev/null", dir);

    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    int total = 0;
    char path[512];
    while (fgets(path, sizeof(path), fp)) {
        char *nl = strchr(path, '\n');
        if (nl) *nl = '\0';
        if (path[0]) {
            int n = policy_load_file(ps, path);
            if (n > 0) total += n;
        }
    }
    pclose(fp);

    log_info(MODULE, "total %d rules loaded from %s", total, dir);
    return total;
}

int policy_add_rule(policy_store_t *ps, const policy_rule_t *rule) {
    if (!ps || !rule) return -1;

    pthread_mutex_lock(&ps->mutex);

    if (ps->count >= MAX_POLICIES) {
        pthread_mutex_unlock(&ps->mutex);
        log_error(MODULE, "policy store full");
        return -1;
    }

    ps->rules[ps->count++] = *rule;
    pthread_mutex_unlock(&ps->mutex);

    log_info(MODULE, "rule added: cap=%s agent=%s action=%s",
             rule->capability, rule->agent_name, policy_action_str(rule->action));
    return 0;
}

int policy_check(policy_store_t *ps, const policy_request_t *req, policy_response_t *resp) {
    if (!ps || !req || !resp) return -1;

    memset(resp, 0, sizeof(*resp));
    resp->id = req->id;
    resp->action = POLICY_DENY;
    snprintf(resp->reason, sizeof(resp->reason), "no matching policy rule");

    pthread_mutex_lock(&ps->mutex);

    policy_rule_t *best = NULL;
    int best_prio = -1;

    for (int i = 0; i < ps->count; i++) {
        policy_rule_t *r = &ps->rules[i];

        if (!match_capability(r->capability, req->capability) && strcmp(r->capability, "*") != 0)
            continue;

        if (!match_agent(r->agent_name, req->agent_name))
            continue;

        if (r->priority > best_prio) {
            best = r;
            best_prio = r->priority;
        }
    }

    if (best) {
        resp->action = best->action;
        snprintf(resp->reason, sizeof(resp->reason), "%s", best->message[0] ? best->message : "");
        snprintf(resp->matched_rule, sizeof(resp->matched_rule), "cap=%s agent=%s",
                 best->capability, best->agent_name);
    }

    pthread_mutex_unlock(&ps->mutex);

    log_debug(MODULE, "policy check: agent=%s cap=%s → %s",
              req->agent_name, req->capability, policy_action_str(resp->action));

    return 0;
}

int policy_reload(policy_store_t *ps, const char *dir) {
    if (!ps || !dir) return -1;

    pthread_mutex_lock(&ps->mutex);
    ps->count = 0;
    pthread_mutex_unlock(&ps->mutex);

    log_info(MODULE, "reloading policies from %s", dir);
    return policy_load_dir(ps, dir);
}

void policy_cleanup(policy_store_t *ps) {
    if (!ps) return;
    pthread_mutex_destroy(&ps->mutex);
    free(ps);
    log_info(MODULE, "policy store cleaned up");
}
