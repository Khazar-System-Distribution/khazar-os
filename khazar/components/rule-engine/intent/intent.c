#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "intent/intent.h"
#include "logger/logger.h"

static intent_entry_t g_intents[MAX_INTENT_ENTRIES];
static int            g_intent_count = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int intent_init(void) {
    memset(g_intents, 0, sizeof(g_intents));
    g_intent_count = 0;
    return 0;
}

int intent_add(const char *query, intent_type_t type, const char *target, const char *action, rule_source_t source) {
    if (g_intent_count >= MAX_INTENT_ENTRIES) return -1;
    pthread_mutex_lock(&g_mutex);
    strncpy(g_intents[g_intent_count].query, query, MAX_QUERY_LEN - 1);
    g_intents[g_intent_count].intent = type;
    strncpy(g_intents[g_intent_count].target, target, MAX_TARGET_LEN - 1);
    strncpy(g_intents[g_intent_count].action, action, MAX_ACTION_LEN - 1);
    g_intents[g_intent_count].source = source;
    g_intent_count++;
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

static void strtolower(char *s) { for (; *s; s++) *s = (char)tolower((unsigned char)*s); }

int intent_lookup(const char *query, match_result_t *result) {
    char query_lower[MAX_QUERY_LEN];
    if (!query || !result) return -1;
    memset(result, 0, sizeof(*result));
    result->intent = INTENT_UNKNOWN;
    strncpy(query_lower, query, sizeof(query_lower) - 1);
    strtolower(query_lower);

    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < g_intent_count; i++) {
        char entry_lower[MAX_QUERY_LEN];
        strncpy(entry_lower, g_intents[i].query, sizeof(entry_lower) - 1);
        strtolower(entry_lower);
        if (strcmp(query_lower, entry_lower) == 0) {
            result->intent = g_intents[i].intent;
            strncpy(result->target, g_intents[i].target, MAX_TARGET_LEN - 1);
            strncpy(result->action, g_intents[i].action, MAX_ACTION_LEN - 1);
            strncpy(result->matched_pattern, g_intents[i].query, MAX_TOKEN_LEN - 1);
            result->confidence = 0.99f;
            result->source = MATCH_SOURCE_INTENT;
            pthread_mutex_unlock(&g_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return -1;
}

void intent_clear_rules(rule_source_t source) {
    pthread_mutex_lock(&g_mutex);
    int write = 0;
    for (int i = 0; i < g_intent_count; i++) {
        if (g_intents[i].source != source) {
            if (i != write) g_intents[write] = g_intents[i];
            write++;
        }
    }
    g_intent_count = write;
    pthread_mutex_unlock(&g_mutex);
}

int intent_load_defaults(void) {
    /* 100+ tətbiq */
    intent_add("firefox aç",       INTENT_OPEN_APPLICATION, "firefox",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("firefox ac",       INTENT_OPEN_APPLICATION, "firefox",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("firefox open",     INTENT_OPEN_APPLICATION, "firefox",       "open", RULE_SOURCE_BUILTIN);
    intent_add("chrome aç",        INTENT_OPEN_APPLICATION, "google-chrome", "aç", RULE_SOURCE_BUILTIN);
    intent_add("telegram aç",      INTENT_OPEN_APPLICATION, "telegram",      "aç", RULE_SOURCE_BUILTIN);
    intent_add("steam aç",         INTENT_OPEN_APPLICATION, "steam",         "aç", RULE_SOURCE_BUILTIN);
    intent_add("spotify aç",       INTENT_OPEN_APPLICATION, "spotify",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("discord aç",       INTENT_OPEN_APPLICATION, "discord",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("vlc aç",           INTENT_OPEN_APPLICATION, "vlc",           "aç", RULE_SOURCE_BUILTIN);
    intent_add("terminal aç",      INTENT_OPEN_APPLICATION, "gnome-terminal","aç", RULE_SOURCE_BUILTIN);
    intent_add("vscode aç",        INTENT_OPEN_APPLICATION, "code",          "aç", RULE_SOURCE_BUILTIN);
    intent_add("gedit aç",         INTENT_OPEN_APPLICATION, "gedit",         "aç", RULE_SOURCE_BUILTIN);
    intent_add("nautilus aç",      INTENT_OPEN_APPLICATION, "nautilus",      "aç", RULE_SOURCE_BUILTIN);
    intent_add("thunderbird aç",   INTENT_OPEN_APPLICATION, "thunderbird",   "aç", RULE_SOURCE_BUILTIN);
    intent_add("gimp aç",          INTENT_OPEN_APPLICATION, "gimp",          "aç", RULE_SOURCE_BUILTIN);
    intent_add("gimp ac",          INTENT_OPEN_APPLICATION, "gimp",          "aç", RULE_SOURCE_BUILTIN);
    intent_add("blender aç",       INTENT_OPEN_APPLICATION, "blender",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("blender ac",       INTENT_OPEN_APPLICATION, "blender",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("libreoffice aç",   INTENT_OPEN_APPLICATION, "libreoffice",   "aç", RULE_SOURCE_BUILTIN);
    intent_add("obs aç",           INTENT_OPEN_APPLICATION, "obs",           "aç", RULE_SOURCE_BUILTIN);
    intent_add("obs ac",           INTENT_OPEN_APPLICATION, "obs",           "aç", RULE_SOURCE_BUILTIN);
    intent_add("kdenlive aç",      INTENT_OPEN_APPLICATION, "kdenlive",      "aç", RULE_SOURCE_BUILTIN);
    intent_add("inkscape aç",      INTENT_OPEN_APPLICATION, "inkscape",      "aç", RULE_SOURCE_BUILTIN);
    intent_add("kate aç",          INTENT_OPEN_APPLICATION, "kate",          "aç", RULE_SOURCE_BUILTIN);
    intent_add("qbittorrent aç",   INTENT_OPEN_APPLICATION, "qbittorrent",   "aç", RULE_SOURCE_BUILTIN);
    intent_add("dolphin aç",       INTENT_OPEN_APPLICATION, "dolphin",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("konsole aç",       INTENT_OPEN_APPLICATION, "konsole",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("okular aç",        INTENT_OPEN_APPLICATION, "okular",        "aç", RULE_SOURCE_BUILTIN);
    intent_add("evince aç",        INTENT_OPEN_APPLICATION, "evince",        "aç", RULE_SOURCE_BUILTIN);
    intent_add("rhythmbox aç",     INTENT_OPEN_APPLICATION, "rhythmbox",     "aç", RULE_SOURCE_BUILTIN);
    intent_add("audacity aç",      INTENT_OPEN_APPLICATION, "audacity",      "aç", RULE_SOURCE_BUILTIN);
    intent_add("krita aç",         INTENT_OPEN_APPLICATION, "krita",         "aç", RULE_SOURCE_BUILTIN);
    intent_add("darktable aç",     INTENT_OPEN_APPLICATION, "darktable",     "aç", RULE_SOURCE_BUILTIN);
    intent_add("handbrake aç",     INTENT_OPEN_APPLICATION, "handbrake",     "aç", RULE_SOURCE_BUILTIN);
    intent_add("virt-manager aç",  INTENT_OPEN_APPLICATION, "virt-manager",  "aç", RULE_SOURCE_BUILTIN);
    intent_add("gnome-boxes aç",   INTENT_OPEN_APPLICATION, "gnome-boxes",   "aç", RULE_SOURCE_BUILTIN);
    intent_add("transmission aç",  INTENT_OPEN_APPLICATION, "transmission",  "aç", RULE_SOURCE_BUILTIN);
    intent_add("filezilla aç",     INTENT_OPEN_APPLICATION, "filezilla",     "aç", RULE_SOURCE_BUILTIN);
    intent_add("flameshot aç",     INTENT_OPEN_APPLICATION, "flameshot",     "aç", RULE_SOURCE_BUILTIN);
    intent_add("keepassxc aç",     INTENT_OPEN_APPLICATION, "keepassxc",     "aç", RULE_SOURCE_BUILTIN);
    intent_add("neovim aç",        INTENT_OPEN_APPLICATION, "nvim",          "aç", RULE_SOURCE_BUILTIN);
    intent_add("vim aç",           INTENT_OPEN_APPLICATION, "vim",           "aç", RULE_SOURCE_BUILTIN);
    intent_add("emacs aç",         INTENT_OPEN_APPLICATION, "emacs",         "aç", RULE_SOURCE_BUILTIN);
    intent_add("htop aç",          INTENT_OPEN_APPLICATION, "htop",          "aç", RULE_SOURCE_BUILTIN);
    intent_add("gnome-disks aç",   INTENT_OPEN_APPLICATION, "gnome-disks",   "aç", RULE_SOURCE_BUILTIN);
    intent_add("baobab aç",        INTENT_OPEN_APPLICATION, "baobab",        "aç", RULE_SOURCE_BUILTIN);
    intent_add("gparted aç",       INTENT_OPEN_APPLICATION, "gparted",       "aç", RULE_SOURCE_BUILTIN);
    intent_add("timeshift aç",     INTENT_OPEN_APPLICATION, "timeshift",     "aç", RULE_SOURCE_BUILTIN);
    intent_add("lutris aç",        INTENT_OPEN_APPLICATION, "lutris",        "aç", RULE_SOURCE_BUILTIN);
    intent_add("minecraft aç",     INTENT_OPEN_APPLICATION, "minecraft",     "aç", RULE_SOURCE_BUILTIN);

    /* Bağla */
    intent_add("firefox bağla",    INTENT_CLOSE_APPLICATION,"firefox",       "bağla", RULE_SOURCE_BUILTIN);
    intent_add("chrome bağla",     INTENT_CLOSE_APPLICATION,"google-chrome", "bağla", RULE_SOURCE_BUILTIN);
    intent_add("telegram bağla",   INTENT_CLOSE_APPLICATION,"telegram",      "bağla", RULE_SOURCE_BUILTIN);
    intent_add("steam bağla",      INTENT_CLOSE_APPLICATION,"steam",         "bağla", RULE_SOURCE_BUILTIN);

    /* Quraşdır */
    intent_add("steam quraşdır",     INTENT_INSTALL_PACKAGE, "steam",         "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("steam qurasdir",     INTENT_INSTALL_PACKAGE, "steam",         "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("steam install",      INTENT_INSTALL_PACKAGE, "steam",         "install",  RULE_SOURCE_BUILTIN);
    intent_add("firefox quraşdır",   INTENT_INSTALL_PACKAGE, "firefox",       "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("chrome quraşdır",    INTENT_INSTALL_PACKAGE, "google-chrome", "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("vscode quraşdır",    INTENT_INSTALL_PACKAGE, "code",          "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("spotify quraşdır",   INTENT_INSTALL_PACKAGE, "spotify",       "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("discord quraşdır",   INTENT_INSTALL_PACKAGE, "discord",       "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("telegram quraşdır",  INTENT_INSTALL_PACKAGE, "telegram",      "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("git quraşdır",       INTENT_INSTALL_PACKAGE, "git",           "quraşdır", RULE_SOURCE_BUILTIN);
    intent_add("docker quraşdır",    INTENT_INSTALL_PACKAGE, "docker",        "quraşdır", RULE_SOURCE_BUILTIN);

    /* Sistem */
    intent_add("sistemi yenilə",    INTENT_SYSTEM_UPDATE,  "system", "yenilə", RULE_SOURCE_BUILTIN);
    intent_add("update system",     INTENT_SYSTEM_UPDATE,  "system", "update", RULE_SOURCE_BUILTIN);
    intent_add("sistemi bağla",     INTENT_SYSTEM_SHUTDOWN,"system", "bağla",  RULE_SOURCE_BUILTIN);
    intent_add("shutdown",          INTENT_SYSTEM_SHUTDOWN,"system", "shutdown",RULE_SOURCE_BUILTIN);
    intent_add("yenidən başlat",    INTENT_SYSTEM_REBOOT,  "system", "reboot", RULE_SOURCE_BUILTIN);
    intent_add("reboot",            INTENT_SYSTEM_REBOOT,  "system", "reboot", RULE_SOURCE_BUILTIN);
    intent_add("ekranı kilidlə",    INTENT_SYSTEM_LOCK,    "system", "lock",   RULE_SOURCE_BUILTIN);
    intent_add("ekrani kilidle",    INTENT_SYSTEM_LOCK,    "system", "lock",   RULE_SOURCE_BUILTIN);
    intent_add("lock",              INTENT_SYSTEM_LOCK,    "system", "lock",   RULE_SOURCE_BUILTIN);
    intent_add("lock screen",       INTENT_SYSTEM_LOCK,    "system", "lock",   RULE_SOURCE_BUILTIN);
    intent_add("çıxış",             INTENT_SYSTEM_LOGOUT,  "system", "logout", RULE_SOURCE_BUILTIN);
    intent_add("logout",            INTENT_SYSTEM_LOGOUT,  "system", "logout", RULE_SOURCE_BUILTIN);

    /* Şəbəkə */
    intent_add("wifi aç",           INTENT_NETWORK_ENABLE, "wifi", "aç",      RULE_SOURCE_BUILTIN);
    intent_add("wifi söndür",       INTENT_NETWORK_DISABLE,"wifi", "söndür",  RULE_SOURCE_BUILTIN);
    intent_add("wifi sondur",       INTENT_NETWORK_DISABLE,"wifi", "söndür",  RULE_SOURCE_BUILTIN);
    intent_add("wifi disable",      INTENT_NETWORK_DISABLE,"wifi", "disable", RULE_SOURCE_BUILTIN);
    intent_add("wifi durum",        INTENT_NETWORK_STATUS, "wifi", "status",  RULE_SOURCE_BUILTIN);

    /* Səs */
    intent_add("sesi artır",        INTENT_VOLUME_UP,      "volume", "up",    RULE_SOURCE_BUILTIN);
    intent_add("volume up",         INTENT_VOLUME_UP,      "volume", "up",    RULE_SOURCE_BUILTIN);
    intent_add("sesi azalt",        INTENT_VOLUME_DOWN,    "volume", "down",  RULE_SOURCE_BUILTIN);
    intent_add("volume down",       INTENT_VOLUME_DOWN,    "volume", "down",  RULE_SOURCE_BUILTIN);
    intent_add("mute",              INTENT_VOLUME_MUTE,    "volume", "mute",  RULE_SOURCE_BUILTIN);

    /* Ekran */
    intent_add("parlaqlığı artır",  INTENT_BRIGHTNESS_UP,  "brightness","up",RULE_SOURCE_BUILTIN);
    intent_add("brightness up",     INTENT_BRIGHTNESS_UP,  "brightness","up",RULE_SOURCE_BUILTIN);
    intent_add("parlaqlığı azalt",  INTENT_BRIGHTNESS_DOWN,"brightness","down",RULE_SOURCE_BUILTIN);
    intent_add("brightness down",   INTENT_BRIGHTNESS_DOWN,"brightness","down",RULE_SOURCE_BUILTIN);
    intent_add("screenshot",        INTENT_SCREENSHOT,     "screen","capture",RULE_SOURCE_BUILTIN);
    intent_add("ekran şəkli çək",   INTENT_SCREENSHOT,     "screen","capture",RULE_SOURCE_BUILTIN);

    /* Fayl */
    intent_add("faylları aç",       INTENT_FILE_OPEN,      "files", "open",   RULE_SOURCE_BUILTIN);

    /* Pəncərə */
    intent_add("pəncərəni kiçilt",  INTENT_WINDOW_MINIMIZE,"window","minimize",RULE_SOURCE_BUILTIN);
    intent_add("pəncərəni böyüt",   INTENT_WINDOW_MAXIMIZE,"window","maximize",RULE_SOURCE_BUILTIN);

    /* Process */
    intent_add("processi öldür",    INTENT_PROCESS_KILL,   "process","kill",   RULE_SOURCE_BUILTIN);

    log_info("intent", "%d intent yuklendi", g_intent_count);
    return 0;
}

void intent_cleanup(void) {
    pthread_mutex_lock(&g_mutex);
    memset(g_intents, 0, sizeof(g_intents));
    g_intent_count = 0;
    pthread_mutex_unlock(&g_mutex);
}
