#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include <pthread.h>

#include "matcher/matcher.h"
#include "logger/logger.h"

static regex_rule_t g_rules[MAX_REGEX_RULES];
static int          g_rule_count = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int matcher_init(void) {
    memset(g_rules, 0, sizeof(g_rules));
    g_rule_count = 0;
    return 0;
}

int matcher_add_rule(const char *pattern, intent_type_t intent, int priority, rule_source_t source) {
    regex_t compiled;
    int rc;

    if (g_rule_count >= MAX_REGEX_RULES) return -1;

    rc = regcomp(&compiled, pattern, REG_EXTENDED | REG_ICASE | REG_NOSUB);
    if (rc != 0) {
        char errbuf[256];
        regerror(rc, &compiled, errbuf, sizeof(errbuf));
        log_error("matcher", "regex compile xatasi '%s': %s", pattern, errbuf);
        return -1;
    }

    pthread_mutex_lock(&g_mutex);
    g_rules[g_rule_count].regex = compiled;
    strncpy(g_rules[g_rule_count].pattern, pattern, MAX_TOKEN_LEN - 1);
    g_rules[g_rule_count].intent = intent;
    g_rules[g_rule_count].priority = priority;
    g_rules[g_rule_count].source = source;
    g_rule_count++;
    pthread_mutex_unlock(&g_mutex);

    return 0;
}

static void extract_target(const char *query, const regex_rule_t *rule, match_result_t *result) {
    const char *p = rule->pattern;
    const char *q = query;

    while (*q && isspace((unsigned char)*q)) q++;
    result->target[0] = '\0';

    if (*p == '^') p++;
    if (strncmp(p, "(.*)", 4) == 0) {
        p += 4;
        while (*q && *p == ' ' && isspace((unsigned char)*q)) { p++; q++; }
        if (*q && *q != ' ') {
            size_t i = 0;
            while (q[i] && !isspace((unsigned char)q[i]) && i < MAX_TARGET_LEN - 1) {
                result->target[i] = q[i];
                i++;
            }
            result->target[i] = '\0';
        }
    }

    while (*p) {
        if (*p == ' ' && isspace((unsigned char)*q)) {
            while (isspace((unsigned char)*q)) q++;
            p++;
            continue;
        }
        if (*p != ' ' && *p != '$' && *p != '(' && *p != ')' && *p != '.' && *p != '*') break;
        p++;
    }

    if (result->target[0] == '\0') strncpy(result->target, query, MAX_TARGET_LEN - 1);
    strncpy(result->action, p, MAX_ACTION_LEN - 1);
}

int matcher_match(const char *query, match_result_t *result) {
    int best_priority = -1, best_idx = -1;

    if (!query || !result) return -1;
    memset(result, 0, sizeof(*result));
    result->intent = INTENT_UNKNOWN;

    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < g_rule_count; i++) {
        if (regexec(&g_rules[i].regex, query, 0, NULL, 0) == 0) {
            if (g_rules[i].priority > best_priority) {
                best_priority = g_rules[i].priority;
                best_idx = i;
            }
        }
    }

    if (best_idx >= 0) {
        result->intent = g_rules[best_idx].intent;
        strncpy(result->matched_pattern, g_rules[best_idx].pattern, MAX_TOKEN_LEN - 1);
        result->confidence = 0.95f;
        result->source = MATCH_SOURCE_REGEX;
        extract_target(query, &g_rules[best_idx], result);
    }
    pthread_mutex_unlock(&g_mutex);

    return (best_idx >= 0) ? 0 : -1;
}

void matcher_clear_rules(rule_source_t source) {
    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < g_rule_count; i++) {
        if (g_rules[i].source == source) {
            regfree(&g_rules[i].regex);
            memset(&g_rules[i], 0, sizeof(regex_rule_t));
        }
    }
    int write = 0;
    for (int i = 0; i < g_rule_count; i++) {
        if (g_rules[i].pattern[0] != '\0') {
            if (i != write) g_rules[write] = g_rules[i];
            write++;
        }
    }
    g_rule_count = write;
    pthread_mutex_unlock(&g_mutex);
}

int matcher_load_defaults(void) {
    matcher_add_rule("^(.*) aç$",             INTENT_OPEN_APPLICATION,  10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) ac$",             INTENT_OPEN_APPLICATION,  10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) open$",           INTENT_OPEN_APPLICATION,  9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) start$",          INTENT_OPEN_APPLICATION,  9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) launch$",         INTENT_OPEN_APPLICATION,  9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) başlat$",         INTENT_OPEN_APPLICATION,  9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) bagla$",          INTENT_CLOSE_APPLICATION, 10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) bağla$",          INTENT_CLOSE_APPLICATION, 10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) close$",          INTENT_CLOSE_APPLICATION, 9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) kapat$",          INTENT_CLOSE_APPLICATION, 9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) söndür$",         INTENT_CLOSE_APPLICATION, 9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) qurasdir$",       INTENT_INSTALL_PACKAGE,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) quraşdır$",       INTENT_INSTALL_PACKAGE,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) install$",        INTENT_INSTALL_PACKAGE,   9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) sil$",            INTENT_REMOVE_PACKAGE,    10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) remove$",         INTENT_REMOVE_PACKAGE,    9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) ara$",            INTENT_SEARCH_PACKAGE,    10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) axtar$",          INTENT_SEARCH_PACKAGE,    10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^(.*) search$",         INTENT_SEARCH_PACKAGE,    9,  RULE_SOURCE_BUILTIN);
    matcher_add_rule("^sistemi yenilə$",      INTENT_SYSTEM_UPDATE,     10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^sistemi yenile$",      INTENT_SYSTEM_UPDATE,     10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^update system$",       INTENT_SYSTEM_UPDATE,     10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^sistemi bağla$",       INTENT_SYSTEM_SHUTDOWN,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^shutdown$",            INTENT_SYSTEM_SHUTDOWN,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^sistemi yenidən başlat$",INTENT_SYSTEM_REBOOT,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^reboot$",              INTENT_SYSTEM_REBOOT,     10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^yuxu moduna keç$",     INTENT_SYSTEM_SUSPEND,    10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^suspend$",             INTENT_SYSTEM_SUSPEND,    10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^ekranı kilidlə$",      INTENT_SYSTEM_LOCK,       10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^lock$",                INTENT_SYSTEM_LOCK,       10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^wifi aç$",             INTENT_NETWORK_ENABLE,    10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^wifi enable$",         INTENT_NETWORK_ENABLE,    10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^wifi söndür$",         INTENT_NETWORK_DISABLE,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^wifi sondur$",         INTENT_NETWORK_DISABLE,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^wifi disable$",        INTENT_NETWORK_DISABLE,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^wifi status$",         INTENT_NETWORK_STATUS,    10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^sesi artır$",          INTENT_VOLUME_UP,         10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^volume up$",           INTENT_VOLUME_UP,         10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^sesi azalt$",          INTENT_VOLUME_DOWN,       10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^volume down$",         INTENT_VOLUME_DOWN,       10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^mute$",                INTENT_VOLUME_MUTE,       10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^parlaqlığı artır$",    INTENT_BRIGHTNESS_UP,     10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^brightness up$",       INTENT_BRIGHTNESS_UP,     10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^parlaqlığı azalt$",    INTENT_BRIGHTNESS_DOWN,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^brightness down$",     INTENT_BRIGHTNESS_DOWN,   10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^screenshot$",          INTENT_SCREENSHOT,        10, RULE_SOURCE_BUILTIN);
    matcher_add_rule("^ekran şəkli çək$",     INTENT_SCREENSHOT,        10, RULE_SOURCE_BUILTIN);

    log_info("matcher", "%d regex qayda yuklendi", g_rule_count);
    return 0;
}

void matcher_cleanup(void) {
    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < g_rule_count; i++) regfree(&g_rules[i].regex);
    memset(g_rules, 0, sizeof(g_rules));
    g_rule_count = 0;
    pthread_mutex_unlock(&g_mutex);
}
