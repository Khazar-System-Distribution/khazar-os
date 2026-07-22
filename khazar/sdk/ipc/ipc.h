#ifndef AI_SDK_IPC_H
#define AI_SDK_IPC_H

#include "../include/common.h"

typedef struct ipc_client ipc_client_t;
typedef struct ipc_server ipc_server_t;

typedef void (*ipc_handler_t)(int fd, const char *data, size_t len, void *ctx);

ipc_client_t *ipc_client_init(const char *socket_path);
int           ipc_client_connect(ipc_client_t *client);
int           ipc_client_send(ipc_client_t *client, const char *data, size_t len);
int           ipc_client_recv(ipc_client_t *client, char *buf, size_t buf_len);
int           ipc_client_recv_timeout(ipc_client_t *client, char *buf, size_t buf_len, int timeout_ms);
void          ipc_client_disconnect(ipc_client_t *client);
void          ipc_client_cleanup(ipc_client_t *client);
int           ipc_client_get_fd(ipc_client_t *client);
bool          ipc_client_is_connected(ipc_client_t *client);

ipc_server_t *ipc_server_init(const char *socket_path, int max_connections);
int           ipc_server_start(ipc_server_t *srv, ipc_handler_t handler, void *ctx);
int           ipc_server_start_async(ipc_server_t *srv, ipc_handler_t handler, void *ctx);
void          ipc_server_stop(ipc_server_t *srv);
void          ipc_server_cleanup(ipc_server_t *srv);
int           ipc_server_send(int fd, const char *data, size_t len);
void          ipc_server_disconnect_fd(int fd);

#endif
