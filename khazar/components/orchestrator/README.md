# AI Orchestrator

Khazar System Distribution AI platformasının mərkəzi koordinasiya daemonudur.

Bütün istifadəçi sorğuları ilk olaraq bu servis tərəfindən qəbul edilir və emal olunur. Orchestrator **heç vaxt** əməliyyatları özü icra etmir — o yalnız koordinasiya edir. Əməliyyatlar agentlər (ayrı repositorilər) tərəfindən həyata keçirilir.

## Funksiyalar

| # | Funksiya | Modul | Texnologiya |
|---|----------|-------|-------------|
| 1 | Unix Domain Socket server | ipc/ | epoll, non-blocking I/O |
| 2 | Request router | router/ | keyword əsaslı dispatcher |
| 3 | Session manager | session/ | context, history, timeout |
| 4 | Agent discovery | registry/ | capability əsaslı lookup |
| 5 | Agent communication | ipc/ + protocol/ + registry/ | JSON-RPC üzərindən |
| 6 | JSON RPC server | protocol/ | request/response/error |
| 7 | Logging | logger/ | 6 səviyyə, thread-safe |
| 8 | Metrics | metrics/ | latency, count, failures |

## Arxitektura

```
┌──────────┐
│  Client  │
└────┬─────┘
     │ Unix Domain Socket
     │ JSON-RPC mesajları
┌────▼──────────────────────────────────────┐
│           AI Orchestrator                  │
│                                            │
│  ┌──────────┐  ┌──────────┐               │
│  │   IPC    │─▶│ Protocol │               │
│  │ (epoll)  │  │ (JSON)   │               │
│  └──────────┘  └────┬─────┘               │
│                     ▼                      │
│              ┌──────────┐                  │
│              │  Router  │──► intent        │
│              └────┬─────┘                  │
│                   ▼                        │
│  ┌──────────┐  ┌──────────┐               │
│  │ Registry │─▶│  Agent   │──► response    │
│  │(discovery)│ │ Comm     │               │
│  └──────────┘  └──────────┘               │
│                                            │
│  ┌──────────┐  ┌──────────┐               │
│  │ Session  │  │ Metrics  │               │
│  └──────────┘  └──────────┘               │
│  ┌──────────┐                              │
│  │  Logger  │◀───────────────              │
│  └──────────┘                              │
└────────────────────────────────────────────┘
     │
     ▼ Agentlər (ayrı repositorilər):
     ai-desktop-agent  ai-package-agent  ai-network-agent  ...
```

## Modullar

### `ipc/` — Unix Domain Socket server
epoll əsaslı, non-blocking I/O ilə UDS server.
- Socket yolu: `/run/ai-orchestrator.sock` (0660)
- Maksimum 256 paralel bağlantı
- Avtomatik təmizləmə (soket faylı silinir)

### `protocol/` — JSON RPC server
Mesaj formatları:
- Ping: `{"type":"ping"}` → `{"status":"alive"}`
- Request: `{"type":"request","query":"..."}` → `{"status":"success","payload":{...}}`
- Register: `{"type":"register","name":"...","capabilities":[...]}` → agent qeydiyyatı
- Error: `{"status":"error","error_code":"..."}`

### `router/` — Request router
İstifadəçi sorğularını keyword əsasında təsnif edir və uyğun capability-ə yönləndirir.
- Sadə keyword matching (aç, bagla, qur, wifi, ...)
- Ağır regex emalı `ai-rule-engine` reposuna aiddir

### `registry/` — Agent discovery
Agentlər startup zamanı özlərini qeydiyyatdan keçirir.
- Agent adı, versiya, socket path, capability-lər
- Capability əsaslı axtarış

### `session/` — Session manager
İstifadəçi sessiyalarının kontekstini və tarixçəsini saxlayır.
- Default timeout: 300 saniyə
- Tarixçə: 50 giriş
- Gələcəkdə anaphora resolution üçün ("onu bağla" → "onu" = Firefox)

### `logger/` — Logging
Thread-safe, 6 səviyyə: TRACE, DEBUG, INFO, WARN, ERROR, FATAL.
Format: `2026-07-14T21:15:31Z INFO module mesaj`

### `metrics/` — Metrics
Toplanan metrikalar: request latency, success/failure count, agent failures.

### `config/` — Konfiqurasiya
TOML formatı: `socket_path`, `max_connections`, `request_timeout_ms`, `enable_metrics`, `log_level`.

## Əlaqəli Repositorilər

| Repo | Funksiya |
|------|----------|
| ai-rule-engine | Regex matching, token lookup, intent table |
| ai-policy-engine | Capability validation, permission checks |
| ai-intent-classifier | LLM əsaslı intent təsnifatı |
| ai-model-runtime | GGUF model inference (llama.cpp) |
| ai-package-agent | Paket idarəetməsi (DNF, Flatpak, AppImage) |
| ai-desktop-agent | Desktop inteqrasiyası (DBus, xdg-open) |
| ai-network-agent | Şəbəkə idarəetməsi (NetworkManager) |
| ai-power-agent | Enerji idarəetməsi (UPower, logind) |
| ai-audio-agent | Səs sistemi (PipeWire, WirePlumber) |
| ai-sdk | Yeni agentlər üçün SDK |
| ai-gui | Rust əsaslı GUI (Iced/Slint) |
| ai-installer | İlkin quraşdırma, model endirmə |

## Build

```bash
make
```

## Test

```bash
echo '{"type":"ping"}' | socat - UNIX-CONNECT:/run/ai-orchestrator.sock
# {"status":"alive"}
```

## Quraşdırma

```bash
sudo make install
ai-orchestrator /etc/ai-orchestrator.toml
```

## Asılılıqlar

- C11 compiler (gcc/clang)
- POSIX threads
- Linux kernel (epoll)
