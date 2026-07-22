#define _GNU_SOURCE
#include "ipc.h"
#include "../logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define MODULE "ipc"

struct ipc_client {
    char socket_path[MAX_SOCKET_LEN];
    int  fd;
    bool connected;
};

ipc_client_t *ipc_client_init(const char *socket_path) {
    ipc_client_t *client = calloc(1, sizeof(ipc_client_t));
    if (!client) return NULL;

    strncpy(client->socket_path,
            socket_path ? socket_path : DEFAULT_ORCHESTRATOR_SOCKET,
            sizeof(client->socket_path) - 1);
    client->fd = -1;
    client->connected = false;

    return client;
}

int ipc_client_connect(ipc_client_t *client) {
    if (!client) return -1;

    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
    }

    client->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client->fd < 0) {
        log_error(MODULE, "socket creation failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", client->socket_path);

    if (connect(client->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error(MODULE, "connect to %s failed: %s", client->socket_path, strerror(errno));
        close(client->fd);
        client->fd = -1;
        return -1;
    }

    client->connected = true;
    log_info(MODULE, "connected to %s", client->socket_path);
    return 0;
}

int ipc_client_send(ipc_client_t *client, const char *data, size_t len) {
    if (!client || !data || client->fd < 0) return -1;

    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(client->fd, data + sent, len - sent);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            log_error(MODULE, "send failed: %s", strerror(errno));
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

int ipc_client_recv(ipc_client_t *client, char *buf, size_t buf_len) {
    return ipc_client_recv_timeout(client, buf, buf_len, REQUEST_TIMEOUT_MS);
}

int ipc_client_recv_timeout(ipc_client_t *client, char *buf, size_t buf_len, int timeout_ms) {
    if (!client || !buf || buf_len == 0 || client->fd < 0) return -1;

    struct pollfd pfd;
    pfd.fd = client->fd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0) {
        log_error(MODULE, "poll failed: %s", strerror(errno));
        return -1;
    }
    if (ret == 0) return 0;

    ssize_t n = read(client->fd, buf, buf_len - 1);
    if (n <= 0) {
        if (n < 0) log_error(MODULE, "recv failed: %s", strerror(errno));
        client->connected = false;
        return -1;
    }

    buf[n] = '\0';
    return (int)n;
}

void ipc_client_disconnect(ipc_client_t *client) {
    if (!client) return;
    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
    }
    client->connected = false;
}

void ipc_client_cleanup(ipc_client_t *client) {
    if (!client) return;
    ipc_client_disconnect(client);
    free(client);
}

int ipc_client_get_fd(ipc_client_t *client) {
    return client ? client->fd : -1;
}

bool ipc_client_is_connected(ipc_client_t *client) {
    return client ? client->connected : false;
}

#define MAX_EPOLL_EVENTS 256

struct ipc_server {
    char socket_path[MAX_SOCKET_LEN];
    int  max_connections;
    int  server_fd;
    int  epoll_fd;
    bool running;
    bool async_mode;
    ipc_handler_t handler;
    void *handler_ctx;
    pthread_t thread;
};

ipc_server_t *ipc_server_init(const char *socket_path, int max_connections) {
    ipc_server_t *srv = calloc(1, sizeof(ipc_server_t));
    if (!srv) return NULL;

    strncpy(srv->socket_path,
            socket_path ? socket_path : DEFAULT_ORCHESTRATOR_SOCKET,
            sizeof(srv->socket_path) - 1);
    srv->max_connections = max_connections > 0 ? max_connections : 256;
    srv->server_fd = -1;
    srv->epoll_fd = -1;
    srv->running = false;

    return srv;
}

static int ipc_server_setup(ipc_server_t *srv) {
    unlink(srv->socket_path);

    srv->server_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (srv->server_fd < 0) {
        log_error(MODULE, "socket creation failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", srv->socket_path);

    if (bind(srv->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error(MODULE, "bind failed: %s", strerror(errno));
        close(srv->server_fd);
        srv->server_fd = -1;
        return -1;
    }

    chmod(srv->socket_path, SOCKET_PERMISSIONS);

    if (listen(srv->server_fd, srv->max_connections) < 0) {
        log_error(MODULE, "listen failed: %s", strerror(errno));
        close(srv->server_fd);
        srv->server_fd = -1;
        unlink(srv->socket_path);
        return -1;
    }

    srv->epoll_fd = epoll_create1(0);
    if (srv->epoll_fd < 0) {
        log_error(MODULE, "epoll creation failed: %s", strerror(errno));
        close(srv->server_fd);
        srv->server_fd = -1;
        unlink(srv->socket_path);
        return -1;
    }

    struct epoll_event ev = {.events = EPOLLIN, .data.fd = srv->server_fd};
    if (epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, srv->server_fd, &ev) < 0) {
        log_error(MODULE, "epoll_ctl failed: %s", strerror(errno));
        close(srv->epoll_fd);
        close(srv->server_fd);
        srv->epoll_fd = -1;
        srv->server_fd = -1;
        unlink(srv->socket_path);
        return -1;
    }

    srv->running = true;
    log_info(MODULE, "server listening on %s", srv->socket_path);
    return 0;
}

static void ipc_event_loop(ipc_server_t *srv) {
    struct epoll_event events[MAX_EPOLL_EVENTS];

    while (srv->running) {
        int nfds = epoll_wait(srv->epoll_fd, events, MAX_EPOLL_EVENTS, 1000);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            log_error(MODULE, "epoll_wait error: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == srv->server_fd) {
                struct sockaddr_un client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept4(srv->server_fd,
                                        (struct sockaddr *)&client_addr, &client_len,
                                        SOCK_NONBLOCK);
                if (client_fd < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                        log_error(MODULE, "accept failed: %s", strerror(errno));
                    continue;
                }

                struct epoll_event cev = {
                    .events = EPOLLIN | EPOLLRDHUP,
                    .data.fd = client_fd
                };
                if (epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, client_fd, &cev) < 0) {
                    close(client_fd);
                    continue;
                }

                log_debug(MODULE, "client connected: fd=%d", client_fd);
            } else {
                int client_fd = events[i].data.fd;

                if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    epoll_ctl(srv->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    continue;
                }

                char buf[MAX_MESSAGE_SIZE];
                ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
                if (n <= 0) {
                    epoll_ctl(srv->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                } else {
                    buf[n] = '\0';
                    if (srv->handler) {
                        srv->handler(client_fd, buf, (size_t)n, srv->handler_ctx);
                    }
                }
            }
        }
    }
}

static void *ipc_async_thread(void *arg) {
    ipc_server_t *srv = (ipc_server_t *)arg;
    ipc_event_loop(srv);
    return NULL;
}

int ipc_server_start(ipc_server_t *srv, ipc_handler_t handler, void *ctx) {
    if (!srv) return -1;

    srv->handler = handler;
    srv->handler_ctx = ctx;
    srv->async_mode = false;

    if (ipc_server_setup(srv) < 0)
        return -1;

    ipc_event_loop(srv);
    return 0;
}

int ipc_server_start_async(ipc_server_t *srv, ipc_handler_t handler, void *ctx) {
    if (!srv) return -1;

    srv->handler = handler;
    srv->handler_ctx = ctx;
    srv->async_mode = true;

    if (ipc_server_setup(srv) < 0)
        return -1;

    if (pthread_create(&srv->thread, NULL, ipc_async_thread, srv) != 0) {
        log_error(MODULE, "failed to create async thread");
        srv->running = false;
        return -1;
    }

    log_info(MODULE, "server started (async) on %s", srv->socket_path);
    return 0;
}

void ipc_server_stop(ipc_server_t *srv) {
    if (!srv) return;
    srv->running = false;
    if (srv->async_mode) {
        pthread_join(srv->thread, NULL);
    }
}

void ipc_server_cleanup(ipc_server_t *srv) {
    if (!srv) return;

    srv->running = false;
    if (srv->async_mode) {
        pthread_join(srv->thread, NULL);
    }

    if (srv->epoll_fd >= 0) close(srv->epoll_fd);
    if (srv->server_fd >= 0) close(srv->server_fd);

    unlink(srv->socket_path);
    free(srv);
    log_info(MODULE, "server cleaned up");
}

void ipc_server_disconnect_fd(int fd) {
    if (fd >= 0) close(fd);
}

int ipc_server_send(int fd, const char *data, size_t len) {
    if (fd < 0 || !data) return -1;

    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, data + sent, len - sent);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}
