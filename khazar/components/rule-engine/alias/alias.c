#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "alias/alias.h"
#include "logger/logger.h"

static alias_entry_t   g_aliases[MAX_ALIASES];
static int             g_alias_count = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int alias_init(void) {
    memset(g_aliases, 0, sizeof(g_aliases));
    g_alias_count = 0;
    return 0;
}

int alias_add(const char *keyword, const char *canonical, rule_source_t source) {
    if (g_alias_count >= MAX_ALIASES) return -1;
    pthread_mutex_lock(&g_mutex);
    strncpy(g_aliases[g_alias_count].alias, keyword, MAX_TOKEN_LEN - 1);
    strncpy(g_aliases[g_alias_count].canonical, canonical, MAX_TOKEN_LEN - 1);
    g_aliases[g_alias_count].source = source;
    g_alias_count++;
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

static void strtolower(char *s) { for (; *s; s++) *s = (char)tolower((unsigned char)*s); }

int alias_resolve(const char *query, match_result_t *result) {
    char query_lower[MAX_QUERY_LEN];

    if (!query || !result) return -1;
    memset(result, 0, sizeof(*result));
    result->intent = INTENT_UNKNOWN;
    strncpy(query_lower, query, sizeof(query_lower) - 1);
    strtolower(query_lower);

    pthread_mutex_lock(&g_mutex);

    for (int i = 0; i < g_alias_count; i++) {
        char alias_lower[MAX_TOKEN_LEN], canon_lower[MAX_TOKEN_LEN];
        strncpy(alias_lower, g_aliases[i].alias, sizeof(alias_lower) - 1);
        strtolower(alias_lower);
        strncpy(canon_lower, g_aliases[i].canonical, sizeof(canon_lower) - 1);
        strtolower(canon_lower);

        /* Partial alias in query */
        char *found = strstr(query_lower, alias_lower);
        if (found) {
            size_t prefix_len = (size_t)(found - query_lower);
            size_t alias_len = strlen(alias_lower);
            if ((prefix_len == 0 || query_lower[prefix_len - 1] == ' ') &&
                (found[alias_len] == '\0' || found[alias_len] == ' ')) {

                strncpy(result->target, g_aliases[i].canonical, MAX_TARGET_LEN - 1);
                strncpy(result->matched_pattern, g_aliases[i].alias, MAX_TOKEN_LEN - 1);
                result->source = MATCH_SOURCE_ALIAS;
                result->confidence = 0.85f;

                /* Build resolved query by replacing alias with canonical */
                char resolved[MAX_QUERY_LEN];
                size_t before = prefix_len;
                memcpy(resolved, query_lower, before);
                size_t canon_sz = strlen(g_aliases[i].canonical);
                memcpy(resolved + before, g_aliases[i].canonical, canon_sz);
                size_t after_start = prefix_len + alias_len;
                strncpy(resolved + before + canon_sz, query_lower + after_start,
                        sizeof(resolved) - before - canon_sz - 1);
                resolved[sizeof(resolved) - 1] = '\0';

                pthread_mutex_unlock(&g_mutex);

                /* Try intent lookup with resolved query - via external call */
                /* We set target and source, caller checks further pipeline */
                return 0;
            }
        }
    }

    pthread_mutex_unlock(&g_mutex);
    return -1;
}

void alias_clear_rules(rule_source_t source) {
    pthread_mutex_lock(&g_mutex);
    int write = 0;
    for (int i = 0; i < g_alias_count; i++) {
        if (g_aliases[i].source != source) {
            if (i != write) g_aliases[write] = g_aliases[i];
            write++;
        }
    }
    g_alias_count = write;
    pthread_mutex_unlock(&g_mutex);
}

int alias_load_defaults(void) {
    alias_add("firefox",   "firefox",        RULE_SOURCE_BUILTIN);
    alias_add("fox",       "firefox",        RULE_SOURCE_BUILTIN);
    alias_add("browser",   "firefox",        RULE_SOURCE_BUILTIN);
    alias_add("brauzer",   "firefox",        RULE_SOURCE_BUILTIN);
    alias_add("chrome",    "google-chrome",  RULE_SOURCE_BUILTIN);
    alias_add("google",    "google-chrome",  RULE_SOURCE_BUILTIN);
    alias_add("telegram",  "telegram",       RULE_SOURCE_BUILTIN);
    alias_add("tg",        "telegram",       RULE_SOURCE_BUILTIN);
    alias_add("steam",     "steam",          RULE_SOURCE_BUILTIN);
    alias_add("oyun",      "steam",          RULE_SOURCE_BUILTIN);
    alias_add("game",      "steam",          RULE_SOURCE_BUILTIN);
    alias_add("terminal",  "gnome-terminal", RULE_SOURCE_BUILTIN);
    alias_add("term",      "gnome-terminal", RULE_SOURCE_BUILTIN);
    alias_add("konsol",    "gnome-terminal", RULE_SOURCE_BUILTIN);
    alias_add("code",      "code",           RULE_SOURCE_BUILTIN);
    alias_add("vscode",    "code",           RULE_SOURCE_BUILTIN);
    alias_add("vs",        "code",           RULE_SOURCE_BUILTIN);
    alias_add("spotify",   "spotify",        RULE_SOURCE_BUILTIN);
    alias_add("music",     "spotify",        RULE_SOURCE_BUILTIN);
    alias_add("musiqi",    "spotify",        RULE_SOURCE_BUILTIN);
    alias_add("discord",   "discord",        RULE_SOURCE_BUILTIN);
    alias_add("chat",      "discord",        RULE_SOURCE_BUILTIN);
    alias_add("vlc",       "vlc",            RULE_SOURCE_BUILTIN);
    alias_add("video",     "vlc",            RULE_SOURCE_BUILTIN);
    alias_add("media",     "vlc",            RULE_SOURCE_BUILTIN);
    alias_add("gedit",     "gedit",          RULE_SOURCE_BUILTIN);
    alias_add("notepad",   "gedit",          RULE_SOURCE_BUILTIN);
    alias_add("nautilus",  "nautilus",       RULE_SOURCE_BUILTIN);
    alias_add("files",     "nautilus",       RULE_SOURCE_BUILTIN);
    alias_add("fayllar",   "nautilus",       RULE_SOURCE_BUILTIN);
    alias_add("thunderbird","thunderbird",   RULE_SOURCE_BUILTIN);
    alias_add("mail",      "thunderbird",    RULE_SOURCE_BUILTIN);
    alias_add("email",     "thunderbird",    RULE_SOURCE_BUILTIN);
    alias_add("gimp",      "gimp",           RULE_SOURCE_BUILTIN);
    alias_add("photoshop", "gimp",           RULE_SOURCE_BUILTIN);
    alias_add("blender",   "blender",        RULE_SOURCE_BUILTIN);
    alias_add("3d",        "blender",        RULE_SOURCE_BUILTIN);
    alias_add("libre",     "libreoffice",    RULE_SOURCE_BUILTIN);
    alias_add("office",    "libreoffice",    RULE_SOURCE_BUILTIN);
    alias_add("obs",       "obs",            RULE_SOURCE_BUILTIN);
    alias_add("yayın",     "obs",            RULE_SOURCE_BUILTIN);
    alias_add("kdenlive",  "kdenlive",       RULE_SOURCE_BUILTIN);
    alias_add("editor",    "kdenlive",       RULE_SOURCE_BUILTIN);
    alias_add("inkscape",  "inkscape",       RULE_SOURCE_BUILTIN);
    alias_add("vektor",    "inkscape",       RULE_SOURCE_BUILTIN);
    alias_add("kate",      "kate",           RULE_SOURCE_BUILTIN);
    alias_add("qbit",      "qbittorrent",    RULE_SOURCE_BUILTIN);
    alias_add("torrent",   "qbittorrent",    RULE_SOURCE_BUILTIN);
    alias_add("docker",    "docker.io",      RULE_SOURCE_BUILTIN);
    alias_add("python",    "python3",        RULE_SOURCE_BUILTIN);
    alias_add("node",      "nodejs",         RULE_SOURCE_BUILTIN);
    alias_add("gcc",       "build-essential",RULE_SOURCE_BUILTIN);
    alias_add("aç",        "aç",             RULE_SOURCE_BUILTIN);
    alias_add("open",      "aç",             RULE_SOURCE_BUILTIN);
    alias_add("start",     "aç",             RULE_SOURCE_BUILTIN);
    alias_add("launch",    "aç",             RULE_SOURCE_BUILTIN);
    alias_add("başlat",    "aç",             RULE_SOURCE_BUILTIN);
    alias_add("bağla",     "bağla",          RULE_SOURCE_BUILTIN);
    alias_add("close",     "bağla",          RULE_SOURCE_BUILTIN);
    alias_add("kapat",     "bağla",          RULE_SOURCE_BUILTIN);
    alias_add("söndür",    "bağla",          RULE_SOURCE_BUILTIN);
    alias_add("kill",      "bağla",          RULE_SOURCE_BUILTIN);
    alias_add("quraşdır",  "quraşdır",       RULE_SOURCE_BUILTIN);
    alias_add("install",   "quraşdır",       RULE_SOURCE_BUILTIN);
    alias_add("yüklə",     "quraşdır",       RULE_SOURCE_BUILTIN);
    alias_add("update",    "sistemi yenilə", RULE_SOURCE_BUILTIN);
    alias_add("upgrade",   "sistemi yenilə", RULE_SOURCE_BUILTIN);
    alias_add("yükselt",   "sistemi yenilə", RULE_SOURCE_BUILTIN);
    alias_add("shutdown",  "sistemi bağla",  RULE_SOURCE_BUILTIN);
    alias_add("poweroff",  "sistemi bağla",  RULE_SOURCE_BUILTIN);
    alias_add("restart",   "yenidən başlat", RULE_SOURCE_BUILTIN);
    alias_add("reboot",    "yenidən başlat", RULE_SOURCE_BUILTIN);

    log_info("alias", "%d alias yuklendi", g_alias_count);
    return 0;
}

void alias_cleanup(void) {
    pthread_mutex_lock(&g_mutex);
    memset(g_aliases, 0, sizeof(g_aliases));
    g_alias_count = 0;
    pthread_mutex_unlock(&g_mutex);
}
