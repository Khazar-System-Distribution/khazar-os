# AI SDK

Khazar AI platforması üçün agent inkişaf SDK-sı. Bütün agentlər eyni IPC protokolundan, eyni mesaj formatından və eyni capability modelindən istifadə edəcəklər.

## Prinsip

Hər agent eyni kodu yenidən yazmaqdansa, SDK-nı include edib birbaşa işləyir:

```c
#include <ai-sdk/agent.h>
#include <ai-sdk/ipc.h>
#include <ai-sdk/protocol.h>
#include <ai-sdk/events.h>
```

## Modullar

| # | Modul | Fayl | Funksiya |
|---|-------|------|----------|
| 1 | `common` | `include/common.h` | Ortaq tip, sabit, error cədvəli |
| 2 | `errors` | `include/errors.h` | Error handling makroları |
| 3 | `json` | `json/` | JSON parse/build/escape utilitləri |
| 4 | `logger` | `logger/` | Thread-safe logging (6 səviyyə) |
| 5 | `protocol` | `protocol/` | Mesaj formatı: parse + build |
| 6 | `ipc` | `ipc/` | Unix Domain Socket client + server (epoll) |
| 7 | `agent` | `agent/` | Agent lifecycle: register, heartbeat, task handler |
| 8 | `events` | `events/` | In-process pub/sub event sistemi |

## IPC Arxitekturası

```
┌──────────────┐     register/heartbeat     ┌─────────────────┐
│   Agent      │ ──────────────────────────  │  Orchestrator   │
│  (SDK user)  │                             │                 │
│              │     task dispatch            │                 │
│  IPC Server  │ ◄────────────────────────── │                 │
└──────────────┘                             └─────────────────┘
     /run/ai-<agent>.sock                     /run/ai-orchestrator.sock
```

Agent iki IPC rabitəsi qurur:
1. **Client** → Orchestrator-a qoşulur (register, heartbeat göndərir)
2. **Server** → Orchestrator-un task göndərməsi üçün dinləyir

## Event Sistemi

In-process pub/sub. Bir agent daxilində modullar arası rabitə üçün:

```c
event_loop_t *el = event_loop_init();
event_subscribe(el, "task.completed", my_handler, NULL);
event_publish(el, "task.completed", "{\"result\":\"ok\"}");
```

## Build

```bash
make          # libai-sdk.a yaradır
make test     # bütün testləri çalışdırır
make install  # /usr/local-ə quraşdırır
```

## İstifadə Nümunəsi

```c
#include <ai-sdk/agent.h>
#include <ai-sdk/logger.h>

static int handle_task(const request_t *req, response_t *resp, void *ctx) {
    resp->id = req->id;
    resp->status = STATUS_SUCCESS;
    snprintf(resp->payload, sizeof(resp->payload), "processed: %s", req->query);
    return 0;
}

int main(void) {
    logger_init(LOG_INFO);

    const char *caps[] = {"open_application", "close_application"};
    agent_config_t config = {
        .name = "desktop-agent",
        .version = "0.1.0",
        .capabilities = {caps[0], caps[1]},
        .cap_count = 2,
        .orchestrator_socket = DEFAULT_ORCHESTRATOR_SOCKET,
        .agent_socket_path = "/run/ai-desktop-agent.sock",
        .task_handler = handle_task,
    };

    agent_t *agent = agent_init(&config);
    agent_register(agent);
    agent_start(agent);
    agent_run(agent);   // bloklayıcı main loop

    agent_cleanup(agent);
    logger_cleanup();
    return 0;
}
```

Kompilyasiya:

```bash
gcc -std=c11 -O2 -o my-agent my-agent.c -I/usr/local/include -L/usr/local/lib -lai-sdk -pthread
```

## Test

```bash
make test
```

```
--- Running Unit Tests ---
PASS: json_extract_string
PASS: json_extract_int
...
All JSON tests passed
All Protocol tests passed
All IPC tests passed
All Agent tests passed
All Events tests passed
--- All tests passed ---
```

## Asılılıqlar

- C11 compiler (gcc/clang)
- POSIX threads (`-pthread`)
- Linux kernel (epoll, Unix domain sockets)

## Əlaqəli Repositorilər

| Repo | Əlaqə |
|------|-------|
| `ai-orchestrator` | Agent-ləri qeydiyyatdan keçirir, task yönləndirir |
| `ai-rule-engine` | Tier 0 inference, intent təyini |
| `ai-policy-engine` | Təhlükəsizlik siyasəti yoxlaması |
| `ai-desktop-agent` | Bu SDK-nı istifadə edən ilk agent |
