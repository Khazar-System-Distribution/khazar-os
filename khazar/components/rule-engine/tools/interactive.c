/*
 * ai-rule-engine Interactive CLI — C əsaslı, UTF-8 təhlükəsiz
 *
 * Usage: ./interactive /tmp/ai-rule.sock
 *   ya da: make interactive
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <termios.h>

#define MAX_BUF 65536
#define COLOR_RESET  "\033[0m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_RED    "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN   "\033[36m"
#define COLOR_BOLD   "\033[1m"

static volatile int running = 1;
static char sock_path[256];

static void sig_handler(int sig) {
    (void)sig;
    running = 0;
}

static int connect_sock(const char *path) {
    struct sockaddr_un addr;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int send_query(const char *query, char *response, size_t respsz) {
    char request[8192];
    int fd, n;

    fd = connect_sock(sock_path);
    if (fd < 0) return -1;

    snprintf(request, sizeof(request),
             "{\"id\":1,\"type\":\"request\",\"query\":\"%s\"}", query);

    if (write(fd, request, strlen(request)) <= 0) { close(fd); return -1; }

    n = (int)read(fd, response, respsz - 1);
    close(fd);

    if (n <= 0) return -1;
    response[n] = '\0';
    return 0;
}

static int ping(const char *path) {
    int fd = connect_sock(path);
    if (fd < 0) return -1;
    const char *msg = "{\"type\":\"ping\"}";
    if (write(fd, msg, strlen(msg)) <= 0) { close(fd); return -1; }
    char buf[512];
    int n = (int)read(fd, buf, sizeof(buf) - 1);
    close(fd);
    return (n > 0 && strstr(buf, "alive")) ? 0 : -1;
}

static void extract_field(const char *json, const char *key, char *out, size_t outsz) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char *p = strstr(json, search);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(search);
    const char *end = strchr(p, '"');
    if (!end) { strncpy(out, p, outsz - 1); return; }
    size_t len = (size_t)(end - p);
    if (len >= outsz) len = outsz - 1;
    memcpy(out, p, len);
    out[len] = '\0';
}

static void extract_float(const char *json, const char *key, float *val) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    *val = p ? (float)atof(p + strlen(search)) : 0.0f;
}

static const char *color_for_source(const char *src) {
    if (strcmp(src, "cache") == 0)  return COLOR_GREEN;
    if (strcmp(src, "intent") == 0) return COLOR_CYAN;
    if (strcmp(src, "regex") == 0)  return COLOR_YELLOW;
    if (strcmp(src, "token") == 0)  return COLOR_GREEN;
    if (strcmp(src, "alias") == 0)  return COLOR_YELLOW;
    if (strcmp(src, "fuzzy") == 0)  return COLOR_RED;
    return COLOR_RESET;
}

static void display_result(const char *query, const char *response) {
    char intent[128] = {0}, target[256] = {0}, action[128] = {0}, src[32] = {0};
    float conf = 0.0f;

    extract_field(response, "intent", intent, sizeof(intent));
    extract_field(response, "target", target, sizeof(target));
    extract_field(response, "action", action, sizeof(action));
    extract_field(response, "source", src, sizeof(src));
    extract_float(response, "confidence", &conf);

    if (strlen(intent) == 0 || strcmp(intent, "unknown") == 0) {
        printf("  " COLOR_RED "✗ tanınmadı" COLOR_RESET "  (query: %s)\n", query);
        return;
    }

    const char *icon = (conf >= 0.90f) ? "✓" : "~";
    printf("  " COLOR_BOLD "%s" COLOR_RESET " " COLOR_GREEN "%s" COLOR_RESET
           " → " COLOR_CYAN "%s" COLOR_RESET
           "  [" COLOR_YELLOW "%.0f%%" COLOR_RESET " | %s%s" COLOR_RESET "]\n",
           icon, intent, target,
           conf * 100,
           color_for_source(src), src);
}

static void print_header(void) {
    printf("\n");
    printf(COLOR_BOLD "╔══════════════════════════════════════════╗\n" COLOR_RESET);
    printf(COLOR_BOLD "║" COLOR_RESET "   AI Rule Engine — Interaktiv CLI  " COLOR_BOLD "║\n" COLOR_RESET);
    printf(COLOR_BOLD "║" COLOR_RESET "   Tier 0 Inference (LLM-free)      " COLOR_BOLD "║\n" COLOR_RESET);
    printf(COLOR_BOLD "╚══════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    printf("  " COLOR_CYAN "firefox aç" COLOR_RESET " | " COLOR_CYAN "wifi söndür" COLOR_RESET
           " | " COLOR_CYAN "steam quraşdır" COLOR_RESET "\n");
    printf("  " COLOR_CYAN "sistemi yenilə" COLOR_RESET " | " COLOR_CYAN "shutdown" COLOR_RESET
           " | " COLOR_CYAN "telegram aç" COLOR_RESET " | " COLOR_CYAN "vlc aç" COLOR_RESET "\n");
    printf("\n  /help = kömək  |  /raw = raw JSON  |  /quit = çıxış\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    char query[2048], response[MAX_BUF];
    int raw_mode = 0;

    if (argc > 1) {
        strncpy(sock_path, argv[1], sizeof(sock_path) - 1);
    } else {
        strncpy(sock_path, "/tmp/ai-rule.sock", sizeof(sock_path) - 1);
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    if (ping(sock_path) != 0) {
        fprintf(stderr, COLOR_RED "✗" COLOR_RESET
                " Rule Engine tapılmadı: %s\n", sock_path);
        fprintf(stderr, "  Əvvəlcə rule engine-i başlat:\n");
        fprintf(stderr, "  cd ai-rule-engine && make interactive\n");
        return 1;
    }

    print_header();

    while (running) {
        printf(COLOR_BOLD "▶ " COLOR_RESET);
        fflush(stdout);

        if (!fgets(query, sizeof(query), stdin)) break;

        size_t len = strlen(query);
        while (len > 0 && (query[len - 1] == '\n' || query[len - 1] == '\r'))
            query[--len] = '\0';
        if (len == 0) continue;

        /* commands */
        if (strcmp(query, "/quit") == 0 || strcmp(query, "/q") == 0 || strcmp(query, "q") == 0)
            break;
        if (strcmp(query, "/help") == 0 || strcmp(query, "/h") == 0) {
            printf("  Nümunələr: firefox aç, wifi söndür, steam quraşdır\n");
            printf("             sistemi yenilə, shutdown, reboot, lock\n");
            printf("             telegram aç, chrome aç, spotify aç, vlc aç\n");
            printf("             gimp aç, blender aç, obs aç, screenshot\n");
            continue;
        }
        if (strcmp(query, "/raw") == 0) {
            raw_mode = !raw_mode;
            printf("  raw mode: %s\n", raw_mode ? "ON" : "OFF");
            continue;
        }

        if (send_query(query, response, sizeof(response)) != 0) {
            printf("  " COLOR_RED "✗ bağlantı xətası" COLOR_RESET "\n");
            continue;
        }

        if (raw_mode) {
            printf("  %s\n", response);
        } else {
            display_result(query, response);
        }
    }

    printf("\n  sağol.\n\n");
    return 0;
}
