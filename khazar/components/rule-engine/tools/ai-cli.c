/*
 * ai-cli — Full chain CLI: Orchestrator → Rule Engine → Agent
 *
 * Orchestrator socket-a bağlanır, sorğu göndərir,
 * cavabı formatlı göstərir.
 *
 * Usage: ./ai-cli /tmp/ai-orch.sock
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#define MAX_BUF 65536
#define C_RESET  "\033[0m"
#define C_GREEN  "\033[32m"
#define C_RED    "\033[31m"
#define C_YELLOW "\033[33m"
#define C_CYAN   "\033[36m"
#define C_BOLD   "\033[1m"

static volatile int running = 1;
static char sock_path[256];

static void sig_handler(int sig) { (void)sig; running = 0; }

static int connect_sock(const char *path) {
    struct sockaddr_un addr;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { close(fd); return -1; }
    return fd;
}

static int send_cmd(const char *path, const char *msg, char *resp, size_t sz) {
    int fd = connect_sock(path);
    if (fd < 0) return -1;
    if (write(fd, msg, strlen(msg)) <= 0) { close(fd); return -1; }
    int n = (int)read(fd, resp, sz - 1);
    close(fd);
    if (n <= 0) return -1;
    resp[n] = '\0';
    return 0;
}

static int ping(const char *path) {
    char buf[512];
    return send_cmd(path, "{\"type\":\"ping\"}", buf, sizeof(buf)) == 0 &&
           strstr(buf, "alive") ? 0 : -1;
}

static void extract(const char *json, const char *key, char *out, size_t outsz) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char *p = strstr(json, search);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(search);
    const char *end = strchr(p, '"');
    if (!end) { strncpy(out, p, outsz - 1); return; }
    size_t len = (size_t)(end - p);
    if (len >= outsz) len = outsz - 1;
    memcpy(out, p, len); out[len] = '\0';
}

int main(int argc, char *argv[]) {
    char query[2048], resp[MAX_BUF];

    if (argc > 1)
        strncpy(sock_path, argv[1], sizeof(sock_path) - 1);
    else
        strncpy(sock_path, "/run/ai-orchestrator.sock", sizeof(sock_path) - 1);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    if (ping(sock_path) != 0) {
        fprintf(stderr, C_RED "✗" C_RESET " Orchestrator tapılmadı: %s\n", sock_path);
        return 1;
    }

    printf("\n" C_BOLD "╔══════════════════════════════════════════╗\n" C_RESET);
    printf(C_BOLD "║" C_RESET "   Khazar AI CLI — Tam Zəncir       " C_BOLD "║\n" C_RESET);
    printf(C_BOLD "║" C_RESET "   Orchestrator → Rule Engine → Agent" C_BOLD "║\n" C_RESET);
    printf(C_BOLD "╚══════════════════════════════════════════╝\n\n" C_RESET);
    printf("  " C_CYAN "firefox aç" C_RESET " | " C_CYAN "wifi söndür" C_RESET " | " C_CYAN "steam quraşdır" C_RESET "\n");
    printf("  /q = çıxış\n\n");

    while (running) {
        printf(C_BOLD "▶ " C_RESET);
        fflush(stdout);
        if (!fgets(query, sizeof(query), stdin)) break;

        size_t len = strlen(query);
        while (len > 0 && (query[len-1] == '\n' || query[len-1] == '\r')) query[--len] = '\0';
        if (len == 0) continue;
        if (strcmp(query, "/q") == 0 || strcmp(query, "q") == 0) break;

        char msg[8192];
        snprintf(msg, sizeof(msg), "{\"id\":1,\"type\":\"request\",\"query\":\"%s\"}", query);

        if (send_cmd(sock_path, msg, resp, sizeof(resp)) != 0) {
            printf("  " C_RED "✗ bağlantı xətası" C_RESET "\n");
            continue;
        }

        /* Parse intent from response */
        char intent[128] = {0}, target[256] = {0};
        extract(resp, "intent", intent, sizeof(intent));
        extract(resp, "target", target, sizeof(target));

        /* Orchestrator wraps differently — check payload message */
        if (intent[0] == '\0') {
            char msg_text[512] = {0};
            extract(resp, "message", msg_text, sizeof(msg_text));
            if (msg_text[0]) {
                if (strstr(msg_text, "no intent matched"))
                    printf("  " C_RED "✗ tanınmadı" C_RESET "\n");
                else
                    printf("  " C_GREEN "✓" C_RESET " %s\n", msg_text);
            } else {
                printf("  %s\n", resp);
            }
        } else {
            printf("  " C_GREEN "✓" C_RESET " %s → " C_CYAN "%s" C_RESET "\n", intent, target);
        }
    }
    printf("\n  sağol.\n\n");
    return 0;
}
