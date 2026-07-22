#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>

#include "../include/common.h"
#include "rules/rules.h"
#include "logger/logger.h"
#include "matcher/matcher.h"
#include "tokens/tokens.h"
#include "intent/intent.h"
#include "alias/alias.h"

static char g_rules_dir[512] = {0};
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int rules_init(const char *rules_dir) {
    if (!rules_dir) return -1;
    pthread_mutex_lock(&g_mutex);
    strncpy(g_rules_dir, rules_dir, sizeof(g_rules_dir) - 1);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

static void trim(char *s) {
    char *start = s, *end;
    while (*start == ' ' || *start == '\t') start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
}

static int parse_intent_type(const char *s) {
    return intent_type_from_str(s);
}

int rules_load_regex(const char *path) {
    FILE *f = fopen(path, "r");
    char line[1024];
    int count = 0;

    if (!f) { log_warn("rules", "regex fayli tapilmadi: %s", path); return 0; }

    while (fgets(line, sizeof(line), f)) {
        char *pattern, *intent_s, *priority_s;
        trim(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        pattern = strtok(line, "|");
        intent_s = strtok(NULL, "|");
        priority_s = strtok(NULL, "|");

        if (!pattern || !intent_s) continue;
        trim(pattern); trim(intent_s);

        int priority = priority_s ? atoi(priority_s) : 10;
        intent_type_t t = parse_intent_type(intent_s);
        if (t == INTENT_UNKNOWN) {
            log_error("rules", "bilinmeyen intent '%s' regexde: %s", intent_s, path);
            continue;
        }

        if (matcher_add_rule(pattern, t, priority, RULE_SOURCE_FILE) == 0) count++;
    }
    fclose(f);
    log_info("rules", "%s: %d regex qayda yuklendi", path, count);
    return count;
}

int rules_load_tokens(const char *path) {
    FILE *f = fopen(path, "r");
    char line[1024];
    int count = 0;

    if (!f) { log_warn("rules", "tokens fayli tapilmadi: %s", path); return 0; }

    while (fgets(line, sizeof(line), f)) {
        char *token, *intent_s, *weight_s;
        trim(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        token = strtok(line, "|");
        intent_s = strtok(NULL, "|");
        weight_s = strtok(NULL, "|");

        if (!token || !intent_s) continue;
        trim(token); trim(intent_s);

        int weight = weight_s ? atoi(weight_s) : 5;
        intent_type_t t = parse_intent_type(intent_s);
        if (t == INTENT_UNKNOWN) {
            log_error("rules", "bilinmeyen intent '%s' tokende: %s", intent_s, path);
            continue;
        }

        if (tokens_add(token, t, weight, RULE_SOURCE_FILE) == 0) count++;
    }
    fclose(f);
    log_info("rules", "%s: %d token yuklendi", path, count);
    return count;
}

int rules_load_intents(const char *path) {
    FILE *f = fopen(path, "r");
    char line[1024];
    int count = 0;

    if (!f) { log_warn("rules", "intent fayli tapilmadi: %s", path); return 0; }

    while (fgets(line, sizeof(line), f)) {
        char *query, *intent_s, *target, *action;
        trim(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        query = strtok(line, "|");
        intent_s = strtok(NULL, "|");
        target = strtok(NULL, "|");
        action = strtok(NULL, "|");

        if (!query || !intent_s) continue;
        trim(query); trim(intent_s);
        if (target) trim(target);
        if (action) trim(action);

        intent_type_t t = parse_intent_type(intent_s);
        if (t == INTENT_UNKNOWN) {
            log_error("rules", "bilinmeyen intent '%s' intentde: %s", intent_s, path);
            continue;
        }

        if (intent_add(query, t, target ? target : "", action ? action : "", RULE_SOURCE_FILE) == 0) count++;
    }
    fclose(f);
    log_info("rules", "%s: %d intent yuklendi", path, count);
    return count;
}

int rules_load_aliases(const char *path) {
    FILE *f = fopen(path, "r");
    char line[1024];
    int count = 0;

    if (!f) { log_warn("rules", "alias fayli tapilmadi: %s", path); return 0; }

    while (fgets(line, sizeof(line), f)) {
        char *alias, *canonical;
        trim(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        alias = strtok(line, "|");
        canonical = strtok(NULL, "|");

        if (!alias || !canonical) continue;
        trim(alias); trim(canonical);

        if (alias_add(alias, canonical, RULE_SOURCE_FILE) == 0) count++;
    }
    fclose(f);
    log_info("rules", "%s: %d alias yuklendi", path, count);
    return count;
}

int rules_load_all(void) {
    char path[1024];
    int total = 0;

    pthread_mutex_lock(&g_mutex);

    if (g_rules_dir[0] == '\0') { pthread_mutex_unlock(&g_mutex); return 0; }

    /* Clear old file rules before reloading */
    matcher_clear_rules(RULE_SOURCE_FILE);
    tokens_clear_rules(RULE_SOURCE_FILE);
    intent_clear_rules(RULE_SOURCE_FILE);
    alias_clear_rules(RULE_SOURCE_FILE);

    snprintf(path, sizeof(path), "%s/regex.rules", g_rules_dir);
    total += rules_load_regex(path);

    snprintf(path, sizeof(path), "%s/tokens.rules", g_rules_dir);
    total += rules_load_tokens(path);

    snprintf(path, sizeof(path), "%s/intent.rules", g_rules_dir);
    total += rules_load_intents(path);

    snprintf(path, sizeof(path), "%s/alias.rules", g_rules_dir);
    total += rules_load_aliases(path);

    pthread_mutex_unlock(&g_mutex);

    log_info("rules", "cemi %d fayl qaydasi yuklendi", total);
    return total;
}

void rules_cleanup(void) {
    pthread_mutex_lock(&g_mutex);
    g_rules_dir[0] = '\0';
    pthread_mutex_unlock(&g_mutex);
}
