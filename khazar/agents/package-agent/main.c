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

#define MODULE "package-agent"
#define AGENT_NAME "package-agent"
#define AGENT_VERSION "0.1.0"
#define ORCH_SOCKET "/tmp/ai-orch.sock"
#define AGENT_SOCKET "/run/ai-package-agent.sock"
#define MAX_RESPONSE 65536

static volatile bool running = true;
static int orch_fd = -1;
static char pkg_manager[16] = "apt";

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
        "\"socket\":\"%s\",\"capabilities\":[\"install_package\",\"remove_package\",\"search_package\",\"system_update\"]}",
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

static void detect_pkg_manager(void) {
    if (access("/usr/bin/pacman", X_OK) == 0) {
        snprintf(pkg_manager, sizeof(pkg_manager), "pacman");
    }
}

static int run_command(const char *cmd) {
    log_info(MODULE, "exec: %s", cmd);
    int ret = system(cmd);
    int status = 0;
    if (ret == -1) {
        log_error(MODULE, "system() failed");
        return -1;
    }
    if (WIFEXITED(ret)) status = WEXITSTATUS(ret);
    return status;
}

static int execute_action(const char *action, const char *target) {
    char cmd[2048];

    if (strcmp(action, "install") == 0) {
        if (!target || !target[0]) return -1;
        if (strcmp(pkg_manager, "pacman") == 0)
            snprintf(cmd, sizeof(cmd), "pacman -S --noconfirm %s", target);
        else
            snprintf(cmd, sizeof(cmd), "apt-get install -y %s", target);
    } else if (strcmp(action, "remove") == 0) {
        if (!target || !target[0]) return -1;
        if (strcmp(pkg_manager, "pacman") == 0)
            snprintf(cmd, sizeof(cmd), "pacman -R --noconfirm %s", target);
        else
            snprintf(cmd, sizeof(cmd), "apt-get remove -y %s", target);
    } else if (strcmp(action, "search") == 0) {
        if (!target || !target[0]) return -1;
        if (strcmp(pkg_manager, "pacman") == 0)
            snprintf(cmd, sizeof(cmd), "pacman -Ss %s", target);
        else
            snprintf(cmd, sizeof(cmd), "apt-cache search %s", target);
    } else if (strcmp(action, "update") == 0) {
        if (strcmp(pkg_manager, "pacman") == 0)
            snprintf(cmd, sizeof(cmd), "pacman -Syu --noconfirm");
        else
            snprintf(cmd, sizeof(cmd), "apt-get update && apt-get upgrade -y");
    } else {
        return -1;
    }

    return run_command(cmd);
}

static void handle_client(int client_fd, const char *data, size_t len) {
    (void)len;
    char resp[MAX_RESPONSE];

    if (strstr(data, "\"type\":\"ping\"")) {
        snprintf(resp, sizeof(resp),
            "{\"id\":0,\"status\":\"success\",\"payload\":{\"message\":\"agent alive\"}}");
        ipc_server_send(client_fd, resp, strlen(resp));
        return;
    }

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

    int success = 0;
    char message[512] = "unknown command";

    if (strstr(query, "install") || strstr(query, "yukle") || strstr(query, "yükle") || strstr(query, "kur")) {
        char pkg[128] = {0};

        const char *keywords[] = {"install", "yukle", "yükle", "kur", NULL};
        for (int i = 0; keywords[i]; i++) {
            const char *pos = strstr(query, keywords[i]);
            if (pos) {
                const char *after = pos + strlen(keywords[i]);
                while (*after == ' ') after++;
                strncpy(pkg, after, sizeof(pkg) - 1);
                break;
            }
        }

        char *t = pkg + strlen(pkg) - 1;
        while (t >= pkg && *t == ' ') { *t = '\0'; t--; }

        if (pkg[0]) {
            if (execute_action("install", pkg) == 0) {
                success = 1;
                snprintf(message, sizeof(message), "installed %s", pkg);
            } else {
                snprintf(message, sizeof(message), "failed to install %s", pkg);
            }
        }
    } else if (strstr(query, "remove") || strstr(query, "kaldir") || strstr(query, "kaldır") || strstr(query, "sil") || strstr(query, "uninstall")) {
        char pkg[128] = {0};

        const char *keywords[] = {"remove", "kaldir", "kaldır", "sil", "uninstall", NULL};
        for (int i = 0; keywords[i]; i++) {
            const char *pos = strstr(query, keywords[i]);
            if (pos) {
                const char *after = pos + strlen(keywords[i]);
                while (*after == ' ') after++;
                strncpy(pkg, after, sizeof(pkg) - 1);
                break;
            }
        }

        char *t = pkg + strlen(pkg) - 1;
        while (t >= pkg && *t == ' ') { *t = '\0'; t--; }

        if (pkg[0]) {
            if (execute_action("remove", pkg) == 0) {
                success = 1;
                snprintf(message, sizeof(message), "removed %s", pkg);
            } else {
                snprintf(message, sizeof(message), "failed to remove %s", pkg);
            }
        }
    } else if (strstr(query, "search") || strstr(query, "ara") || strstr(query, "find")) {
        char pkg[128] = {0};

        const char *keywords[] = {"search", "ara", "find", NULL};
        for (int i = 0; keywords[i]; i++) {
            const char *pos = strstr(query, keywords[i]);
            if (pos) {
                const char *after = pos + strlen(keywords[i]);
                while (*after == ' ') after++;
                strncpy(pkg, after, sizeof(pkg) - 1);
                break;
            }
        }

        char *t = pkg + strlen(pkg) - 1;
        while (t >= pkg && *t == ' ') { *t = '\0'; t--; }

        if (pkg[0]) {
            execute_action("search", pkg);
            success = 1;
            snprintf(message, sizeof(message), "searched for %s", pkg);
        }
    } else if (strstr(query, "update") || strstr(query, "guncelle") || strstr(query, "güncelle") || strstr(query, "upgrade")) {
        if (execute_action("update", NULL) == 0) {
            success = 1;
            snprintf(message, sizeof(message), "system updated");
        } else {
            snprintf(message, sizeof(message), "update failed");
        }
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
    log_info(MODULE, "AI Package Agent v%s starting...", AGENT_VERSION);

    detect_pkg_manager();
    log_info(MODULE, "detected package manager: %s", pkg_manager);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    unlink(AGENT_SOCKET);

    if (register_with_orchestrator() < 0) {
        log_warn(MODULE, "registration failed - orchestrator may not be running");
        log_warn(MODULE, "will retry...");
    }

    ipc_server_t *srv = ipc_server_init(AGENT_SOCKET, 64);
    if (!srv) {
        log_fatal(MODULE, "failed to init IPC server");
    }

    log_info(MODULE, "agent ready on %s", AGENT_SOCKET);
    log_info(MODULE, "capabilities: install_package, remove_package, search_package, system_update");
    log_info(MODULE, "waiting for tasks... (Ctrl+C to stop)");

    int server_fd = -1;
    {
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
