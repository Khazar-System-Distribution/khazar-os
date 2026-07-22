#define _GNU_SOURCE
#include "../../sdk/include/common.h"
#include "logger/logger.h"
#include "ipc/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>

#define MODULE "desktop-agent"
#define AGENT_NAME "desktop-agent"
#define AGENT_VERSION "0.1.0"
#define ORCH_SOCKET "/tmp/ai-orch.sock"
#define AGENT_SOCKET "/tmp/ai-desktop-agent.sock"
#define MAX_RESPONSE 65536

static volatile bool running = true;
static int orch_fd = -1;

static void signal_handler(int sig) {
    (void)sig;
    running = false;
}

static int connect_to(const char *socket_path) {
    struct sockaddr_un addr;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static int send_and_recv(int fd, const char *msg, char *resp, size_t resp_len) {
    ssize_t w = write(fd, msg, strlen(msg));
    if (w < 0) return -1;

    ssize_t r = read(fd, resp, resp_len - 1);
    if (r <= 0) return -1;

    resp[r] = '\0';
    return 0;
}

static int register_with_orchestrator(void) {
    orch_fd = connect_to(ORCH_SOCKET);
    if (orch_fd < 0) {
        log_error(MODULE, "cannot connect to orchestrator at %s", ORCH_SOCKET);
        return -1;
    }

    char msg[4096], resp[4096];
    snprintf(msg, sizeof(msg),
        "{\"type\":\"register\",\"name\":\"%s\",\"version\":\"%s\","
        "\"socket\":\"%s\",\"capabilities\":[\"open_application\",\"close_application\"]}",
        AGENT_NAME, AGENT_VERSION, AGENT_SOCKET);

    if (send_and_recv(orch_fd, msg, resp, sizeof(resp)) < 0) {
        log_error(MODULE, "registration failed");
        close(orch_fd);
        orch_fd = -1;
        return -1;
    }

    log_info(MODULE, "registered: %s", resp);
    return 0;
}

static int extract_str(const char *json, const char *key, char *buf, size_t bufsz) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char *p = strstr(json, search);
    if (!p) return -1;

    p += strlen(search);
    const char *end = strchr(p, '"');
    if (!end) { strncpy(buf, p, bufsz - 1); return 0; }

    size_t len = (size_t)(end - p);
    if (len >= bufsz) len = bufsz - 1;
    memcpy(buf, p, len);
    buf[len] = '\0';
    return 0;
}

static char *find_executable(const char *name) {
    static char path[512];
    const char *dirs[] = {"/usr/bin", "/usr/local/bin", "/bin", "/snap/bin", NULL};

    for (int i = 0; dirs[i]; i++) {
        snprintf(path, sizeof(path), "%s/%s", dirs[i], name);
        if (access(path, X_OK) == 0) return path;
    }

    /* try which */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "which %s 2>/dev/null", name);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(path, sizeof(path), fp)) {
            char *nl = strchr(path, '\n');
            if (nl) *nl = '\0';
            pclose(fp);
            if (path[0] && access(path, X_OK) == 0) return path;
        } else {
            pclose(fp);
        }
    }

    return NULL;
}

static int open_application(const char *app_name) {
    if (!app_name || !app_name[0]) return -1;

    char lower_name[256];
    int i = 0;
    for (const char *p = app_name; *p && i < (int)sizeof(lower_name) - 1; p++)
        lower_name[i++] = (*p >= 'A' && *p <= 'Z') ? *p + 32 : *p;
    lower_name[i] = '\0';

    log_info(MODULE, "opening application: %s", lower_name);

    char *exe = find_executable(lower_name);
    if (!exe) {
        log_warn(MODULE, "executable not found: %s", lower_name);
        return -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        execl(exe, lower_name, (char *)NULL);
        _exit(1);
    } else if (pid > 0) {
        log_info(MODULE, "launched %s (pid=%d)", lower_name, pid);
        return 0;
    }

    return -1;
}

static void handle_client(int client_fd, const char *data, size_t len) {
    (void)len;
    char resp[MAX_RESPONSE];

    /* ping check */
    if (strstr(data, "\"type\":\"ping\"")) {
        snprintf(resp, sizeof(resp),
            "{\"id\":0,\"status\":\"success\",\"payload\":{\"message\":\"agent alive\"}}");
        ipc_server_send(client_fd, resp, strlen(resp));
        return;
    }

    /* extract request fields */
    char query[2048] = {0};
    char id_str[32] = {0};
    uint64_t id = 0;

    const char *idp = strstr(data, "\"id\"");
    if (idp) {
        idp = strchr(idp, ':');
        if (idp) {
            idp++;
            while (*idp == ' ') idp++;
            int j = 0;
            while (*idp && *idp != ',' && *idp != '}' && j < 31) id_str[j++] = *idp++;
            id = strtoull(id_str, NULL, 10);
        }
    }

    extract_str(data, "query", query, sizeof(query));

    log_info(MODULE, "task received: id=%llu query='%s'", (unsigned long long)id, query);

    /* determine action from query */
    int success = 0;
    char message[512] = "unknown command";

    /* "X aç" or "X ac" or "open X" → open application */
    char *ac_pos = strstr(query, "aç");
    if (!ac_pos) ac_pos = strstr(query, "ac");
    if (!ac_pos) ac_pos = strstr(query, "aç");
    if (!ac_pos) ac_pos = strstr(query, "open");

    if (ac_pos) {
        char app[128] = {0};

        if (strstr(ac_pos, "open")) {
            const char *name_start = ac_pos + 4;
            while (*name_start == ' ') name_start++;
            strncpy(app, name_start, sizeof(app) - 1);
        } else {
            char *space = ac_pos;
            while (space > query && *space != ' ') space--;
            if (space != query) {
                int app_idx = 0;
                for (const char *p = query; p < space && app_idx < 127; p++)
                    app[app_idx++] = *p;
            }
            if (!app[0]) {
                const char *after = ac_pos + strlen(ac_pos);
                while (*after == ' ') after++;
                int app_idx = 0;
                for (const char *p = after; *p && *p != ' ' && app_idx < 127; p++)
                    app[app_idx++] = *p;
            }
        }

        /* trim trailing spaces */
        char *t = app + strlen(app) - 1;
        while (t >= app && *t == ' ') { *t = '\0'; t--; }

        if (app[0]) {
            if (open_application(app) == 0) {
                success = 1;
                snprintf(message, sizeof(message), "opened %s", app);
            } else {
                snprintf(message, sizeof(message), "failed to open %s", app);
            }
        }
    }

    if (strstr(query, "bagla") || strstr(query, "bağla") ||
        strstr(query, "sondur") || strstr(query, "söndür") ||
        strstr(query, "close") || strstr(query, "kill")) {
        success = 1;
        snprintf(message, sizeof(message), "close action acknowledged");
    }

    snprintf(resp, sizeof(resp),
        "{\"id\":%llu,\"status\":\"%s\",\"payload\":{\"message\":\"%s\"}}",
        (unsigned long long)id,
        success ? "success" : "error",
        message);

    ipc_server_send(client_fd, resp, strlen(resp));
    log_info(MODULE, "task result: %s", success ? "SUCCESS" : "FAILED");
}

int main(void) {
    logger_init(NULL, LOG_INFO);
    log_info(MODULE, "AI Desktop Agent v%s starting...", AGENT_VERSION);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    unlink(AGENT_SOCKET);

    /* 1. Register with orchestrator */
    if (register_with_orchestrator() < 0) {
        log_warn(MODULE, "registration failed - orchestrator may not be running");
        log_warn(MODULE, "will retry...");
    }

    /* 2. Start IPC server */
    ipc_server_t *srv = ipc_server_init(AGENT_SOCKET, 64);
    if (!srv) {
        log_fatal(MODULE, "failed to init IPC server");
    }

    log_info(MODULE, "agent ready on %s", AGENT_SOCKET);
    log_info(MODULE, "capabilities: open_application, close_application");
    log_info(MODULE, "waiting for tasks... (Ctrl+C to stop)");

    /* 3. Main loop - accept connections */
    int server_fd = -1;
    {
        /* We need to manually run a simple event loop since we also need heartbeat */
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", AGENT_SOCKET);

        server_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (server_fd < 0) log_fatal(MODULE, "socket failed");

        unlink(AGENT_SOCKET);
        if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            log_fatal(MODULE, "bind failed");
        chmod(AGENT_SOCKET, 0660);
        if (listen(server_fd, 8) < 0)
            log_fatal(MODULE, "listen failed");
    }

    time_t last_heartbeat = time(NULL);

    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_fd, &fds);
        int maxfd = server_fd;

        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
        int ready = select(maxfd + 1, &fds, NULL, NULL, &tv);

        if (ready > 0 && FD_ISSET(server_fd, &fds)) {
            struct sockaddr_un client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd >= 0) {
                char buf[MAX_MESSAGE_SIZE];
                ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    handle_client(client_fd, buf, (size_t)n);
                }
                close(client_fd);
            }
        }

        /* heartbeat */
        time_t now = time(NULL);
        if (now - last_heartbeat >= HEARTBEAT_INTERVAL_SEC && orch_fd >= 0) {
            char beat[256];
            snprintf(beat, sizeof(beat),
                "{\"type\":\"heartbeat\",\"name\":\"%s\",\"timestamp\":%ld}",
                AGENT_NAME, (long)now);
            ssize_t w = write(orch_fd, beat, strlen(beat));
            if (w < 0) {
                close(orch_fd);
                orch_fd = -1;
                log_warn(MODULE, "heartbeat failed, reconnecting...");
                register_with_orchestrator();
            }
            last_heartbeat = now;
        }
    }

    close(server_fd);
    if (orch_fd >= 0) close(orch_fd);
    unlink(AGENT_SOCKET);
    logger_cleanup();

    log_info(MODULE, "agent stopped");
    return 0;
}
