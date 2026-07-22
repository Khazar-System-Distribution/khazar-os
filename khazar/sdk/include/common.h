#ifndef AI_SDK_COMMON_H
#define AI_SDK_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <sys/types.h>

#define AI_SDK_VERSION         "0.1.0"

#define MAX_MESSAGE_SIZE       65536
#define MAX_AGENTS             64
#define MAX_CAPABILITIES       32
#define MAX_NAME_LEN           64
#define MAX_SOCKET_LEN         256
#define MAX_EVENTS             64

#define DEFAULT_ORCHESTRATOR_SOCKET  "/run/ai-orchestrator.sock"
#define HEARTBEAT_INTERVAL_SEC       5
#define HEARTBEAT_TIMEOUT_SEC        15
#define REQUEST_TIMEOUT_MS           3000
#define SOCKET_PERMISSIONS           0660

typedef enum {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} log_level_t;

typedef enum {
    STATUS_SUCCESS,
    STATUS_ERROR
} status_t;

typedef enum {
    ERR_NONE = 0,
    ERR_AGENT_TIMEOUT,
    ERR_AGENT_DISCONNECTED,
    ERR_POLICY_DENIED,
    ERR_INTERNAL,
    ERR_INVALID_REQUEST,
    ERR_AGENT_UNAVAILABLE,
    ERR_SYSTEM,
    ERR_NOT_REGISTERED,
    ERR_REGISTRATION_FAILED,
    ERR_CONNECTION_FAILED,
    ERR_PROTOCOL_ERROR,
    ERR_TIMEOUT
} error_code_t;

typedef enum {
    PRIORITY_CRITICAL = 0,
    PRIORITY_HIGH = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_BACKGROUND = 3
} priority_t;

typedef enum {
    AGENT_STATE_UNREGISTERED,
    AGENT_STATE_REGISTERING,
    AGENT_STATE_REGISTERED,
    AGENT_STATE_RUNNING,
    AGENT_STATE_ERROR,
    AGENT_STATE_SHUTDOWN
} agent_state_t;

typedef struct {
    uint64_t    id;
    char        query[MAX_MESSAGE_SIZE];
    char        client_addr[64];
    time_t      received_at;
    priority_t  priority;
    int         client_fd;
} request_t;

typedef struct {
    uint64_t     id;
    status_t     status;
    char         payload[MAX_MESSAGE_SIZE];
    error_code_t error_code;
} response_t;

typedef struct {
    char name[MAX_NAME_LEN];
    char version[16];
    char socket_path[MAX_SOCKET_LEN];
    char capabilities[MAX_CAPABILITIES][MAX_NAME_LEN];
    int  cap_count;
    bool alive;
    time_t last_heartbeat;
} agent_info_t;

typedef struct {
    char action[64];
    char target[128];
    char required_capability[64];
    char parameters[MAX_MESSAGE_SIZE];
} intent_t;

static inline const char *error_code_str(error_code_t code) {
    switch (code) {
        case ERR_NONE:                return "NONE";
        case ERR_AGENT_TIMEOUT:       return "AGENT_TIMEOUT";
        case ERR_AGENT_DISCONNECTED:  return "AGENT_DISCONNECTED";
        case ERR_POLICY_DENIED:       return "POLICY_DENIED";
        case ERR_INTERNAL:            return "INTERNAL_ERROR";
        case ERR_INVALID_REQUEST:     return "INVALID_REQUEST";
        case ERR_AGENT_UNAVAILABLE:   return "AGENT_UNAVAILABLE";
        case ERR_SYSTEM:              return "SYSTEM_ERROR";
        case ERR_NOT_REGISTERED:      return "NOT_REGISTERED";
        case ERR_REGISTRATION_FAILED: return "REGISTRATION_FAILED";
        case ERR_CONNECTION_FAILED:   return "CONNECTION_FAILED";
        case ERR_PROTOCOL_ERROR:      return "PROTOCOL_ERROR";
        case ERR_TIMEOUT:             return "TIMEOUT";
        default:                      return "UNKNOWN";
    }
}

static inline const char *agent_state_str(agent_state_t state) {
    switch (state) {
        case AGENT_STATE_UNREGISTERED: return "UNREGISTERED";
        case AGENT_STATE_REGISTERING:  return "REGISTERING";
        case AGENT_STATE_REGISTERED:   return "REGISTERED";
        case AGENT_STATE_RUNNING:      return "RUNNING";
        case AGENT_STATE_ERROR:        return "ERROR";
        case AGENT_STATE_SHUTDOWN:     return "SHUTDOWN";
        default:                       return "UNKNOWN";
    }
}

#endif
