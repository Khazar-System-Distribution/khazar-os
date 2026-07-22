# IPC Protocol

## Nəqliyyat (Transport)

| Xüsusiyyət | Dəyər |
|-----------|-------|
| Növ | Unix Domain Sockets (`AF_UNIX`, `SOCK_STREAM`) |
| Format | JSON, newline-delimited |
| Maksimum ölçü | 65536 bayt (64 KB) |
| İcazə | 0660 (owner+group read/write) |
| Kodlaşdırma | UTF-8 |

## Socket Yolları

| Servis | Socket |
|--------|--------|
| Orchestrator | `/run/ai-orchestrator.sock` |
| Rule Engine | `/run/ai-rule-engine.sock` |
| Policy Engine | `/run/ai-policy-engine.sock` |
| Model Runtime | `/run/ai-model-runtime.sock` |
| Intent Classifier | `/run/ai-intent-classifier.sock` |
| Agent (şablon) | `/run/ai-{agent-name}.sock` |

## Mesaj Tipləri

Bütün mesajlar `type` sahəsi ilə identifikasiya olunur:

| # | `type` | Göndərən → Alıcı | Məqsəd |
|---|--------|------------------|--------|
| 1 | `ping` | Hər kəs → Hər kəs | Canlılıq yoxlaması |
| 2 | `pong` | Hər kəs → Hər kəs | Ping-ə cavab |
| 3 | `request` | Client → Orchestrator | İstifadəçi sorğusu |
| 4 | `response` | Hər kəs → Hər kəs | Sorğu nəticəsi |
| 5 | `register` | Agent → Orchestrator | Agent qeydiyyatı |
| 6 | `unregister` | Agent → Orchestrator | Agent çıxışı |
| 7 | `heartbeat` | Agent → Orchestrator | Canlı qalma siqnalı |
| 8 | `event` | Hər kəs → Hər kəs | Hadisə bildirişi |
| 9 | `error` | Hər kəs → Hər kəs | Səhv mesajı |
| 10 | `version` | Hər kəs → Servis | Versiya sorğusu |
| 11 | `metrics` | Hər kəs → Servis | Metrika sorğusu |

### 1. PING

Sağlamlıq yoxlaması. Orchestrator cavab formatı Rule Engine-dən fərqlidir.

**Sorğu:**
```json
{"type": "ping"}
```

**Cavab (Orchestrator):**
```json
{"status": "alive"}
```

**Cavab (Rule Engine):**
```json
{"type": "response", "status": "alive"}
```

### 2. PONG

Ping-ə asinxron cavab. Yalnız növ aşkarlanması üçün istifadə olunur.

```json
{"type": "pong"}
```

### 3. REQUEST

İstifadəçi sorğusu. Orchestrator-a göndərilir, oradan Rule Engine-ə yönləndirilir.

**Client → Orchestrator:**
```json
{
    "id": 1,
    "type": "request",
    "query": "firefox aç"
}
```

**Orchestrator → Rule Engine (eyni format):**
```json
{
    "id": 1,
    "type": "request",
    "query": "firefox aç"
}
```

| Sahə | Növ | Məcburi | İzah |
|------|-----|---------|------|
| `id` | uint64 | Bəli | Request ID (cavabla cütləşmə üçün) |
| `type` | string | Bəli | `"request"` |
| `query` | string | Bəli | İstifadəçi əmri (maks. 2048 simvol) |

### 4. RESPONSE

Sorğu nəticəsi. İki əsas format var: **uğurlu cavab** və **Rule Engine intent cavabı**.

**Uğurlu cavab:**
```json
{
    "id": 42,
    "status": "success",
    "payload": {
        "message": "routed to open_application",
        "agent": "desktop-agent",
        "target": "/run/ai-desktop-agent.sock"
    }
}
```

**Səhv cavabı:**
```json
{
    "id": 42,
    "status": "error",
    "error_code": "AGENT_UNAVAILABLE",
    "payload": {
        "message": "no agent found for open_application"
    }
}
```

**Rule Engine intent cavabı (uğurlu):**
```json
{
    "id": 1,
    "type": "response",
    "intent": "open_application",
    "target": "firefox",
    "action": "aç",
    "confidence": 1.0,
    "source": "regex",
    "matched": "^(.*) aç$"
}
```

**Rule Engine intent cavabı (tanınmadı):**
```json
{
    "id": 1,
    "type": "response",
    "intent": "unknown",
    "confidence": 0.0,
    "source": "none"
}
```

**Rule Engine multi-intent cavabı:**
```json
{
    "id": 1,
    "type": "response",
    "multi": true,
    "results": [
        {
            "intent": "open_application",
            "target": "firefox",
            "action": "aç",
            "confidence": 0.95,
            "source": "regex"
        },
        {
            "intent": "open_application",
            "target": "telegram",
            "action": "aç",
            "confidence": 0.92,
            "source": "regex"
        }
    ]
}
```

| Sahə | Növ | İzah |
|------|-----|------|
| `id` | uint64 | Uyğun request ID-si |
| `type` | string | `"response"` |
| `status` | string | `"success"` və ya `"error"` |
| `intent` | string | Tapılmış intent adı |
| `target` | string | Hədəf (məs. `"firefox"`) |
| `action` | string | Əmr feli (məs. `"aç"`) |
| `confidence` | float | 0.0–1.0 inam dərəcəsi |
| `source` | string | Uyğunluq mənbəyi (aşağıya bax) |
| `matched` | string | Uyğun gələn pattern |
| `multi` | bool | `true` = çoxlu nəticə |
| `results` | array | Çoxlu nəticə massivi |
| `error_code` | string | Səhv kodu (`error-codes.md`-ə bax) |

**Source dəyərləri:**

| `source` | Mərhələ | Sürət | Metod |
|----------|---------|-------|-------|
| `cache` | Tier 0.1 | O(1) | Hash lookup (son 256 əmr) |
| `regex` | Tier 0.2 | O(n) | POSIX regex pattern matching |
| `token` | Tier 0.3 | O(n) | Açar söz lookup + scoring |
| `intent` | Tier 0.4 | O(n) | Tam əmr → intent cədvəli |
| `alias` | Tier 0.5 | O(n) | Alternativ ad həlli |
| `fuzzy` | Tier 0.6 | O(n²) | Təxmini uyğunluq (Levenshtein) |
| `none` | — | — | Tapılmadı (Tier 1-ə keç) |

### 5. REGISTER

Agent işə düşəndə Orchestrator-a özünü qeydiyyatdan keçirir.

```json
{
    "type": "register",
    "name": "desktop-agent",
    "version": "0.1.0",
    "capabilities": [
        "open_application",
        "close_application"
    ]
}
```

| Sahə | Növ | Məcburi | İzah |
|------|-----|---------|------|
| `type` | string | Bəli | `"register"` |
| `name` | string | Bəli | Agent adı (maks. 64 simvol) |
| `version` | string | Bəli | Semantic version (maks. 16 simvol) |
| `capabilities` | string[] | Bəli | Bacarıq siyahısı (maks. 32 ədəd) |

### 6. UNREGISTER

Agent təmiz bağlananda göndərir.

```json
{
    "type": "unregister"
}
```

### 7. HEARTBEAT

Agent hər 5 saniyədən bir canlı qalma siqnalı göndərir.
15 saniyə ərzində heartbeat gəlməzsə, agent ölü sayılır.

```json
{
    "type": "heartbeat",
    "name": "desktop-agent",
    "timestamp": 1718234567
}
```

| Sahə | Növ | İzah |
|------|-----|------|
| `type` | string | `"heartbeat"` |
| `name` | string | Agent adı |
| `timestamp` | int64 | Unix timestamp (saniyə) |

**Timeout dəyərləri:**

| Parametr | Dəyər |
|----------|-------|
| Heartbeat interval | 5 saniyə |
| Heartbeat timeout | 15 saniyə |
| Request timeout | 3000 ms |

### 8. EVENT

Hadisə əsaslı rabitə. Həm agent daxilində (in-process pub/sub),
həm də komponentlər arası istifadə oluna bilər.

```json
{
    "type": "event",
    "event_type": "task.completed",
    "data": {
        "task_id": 42,
        "result": "success"
    }
}
```

| Sahə | Növ | İzah |
|------|-----|------|
| `type` | string | `"event"` |
| `event_type` | string | Hadisə adı (maks. 64 simvol) |
| `data` | any | Hadisə məlumatı |

### 9. ERROR

Aşkar səhv mesajı.

**SDK formatı:**
```json
{
    "id": 7,
    "status": "error",
    "error_code": "TIMEOUT",
    "payload": {
        "message": "agent did not respond within 3000ms"
    }
}
```

**Rule Engine formatı:**
```json
{
    "id": 1,
    "type": "error",
    "error_code": "INVALID_REQUEST"
}
```

### 10. VERSION

Servis versiya məlumatı sorğusu.

**Sorğu:**
```json
{"type": "version"}
```

**Cavab:**
```json
{
    "type": "response",
    "version": "0.3.0",
    "uptime": 3600,
    "cache_size": 128,
    "tier": 0
}
```

| Sahə | Növ | İzah |
|------|-----|------|
| `version` | string | Semantic version |
| `uptime` | int64 | İşləmə müddəti (saniyə) |
| `cache_size` | int | Keşdəki əmr sayı |
| `tier` | int | Tier səviyyəsi (0 = Rule Engine, 1 = LLM) |

### 11. METRICS

Pipeline performans statistikası sorğusu.

**Sorğu:**
```json
{"type": "metrics"}
```

**Cavab:**
```json
{
    "type": "response",
    "metrics": {
        "total_requests": 1500,
        "cache_hits": 1200,
        "regex_hits": 150,
        "token_hits": 80,
        "intent_hits": 40,
        "alias_hits": 20,
        "fuzzy_hits": 5,
        "unknown_hits": 5,
        "multi_intents": 8,
        "cache_hit_ratio": 0.80,
        "avg_latency_us": 125.5,
        "max_latency_us": 5400.0,
        "uptime_sec": 86400,
        "active_connections": 3,
        "cache_size": 128
    }
}
```

## Request Lifecycle

Tam istifadəçi sorğusunun keçdiyi yol:

```
Client
  │
  │  {"type":"request","id":1,"query":"firefox aç"}
  ▼
┌────────────────────┐
│   Orchestrator     │  /run/ai-orchestrator.sock
│                    │
│  1. Parse sorğu    │
│  2. Tier 0-a göndər│
└────────┬───────────┘
         │
         │  {"type":"request","id":1,"query":"firefox aç"}
         ▼
┌────────────────────┐
│   Rule Engine      │  /run/ai-rule-engine.sock
│   Tier 0 Pipeline  │
│                    │
│  Cache → Regex →   │
│  Token → Intent →  │
│  Alias → Fuzzy     │
└────────┬───────────┘
         │
    ┌────┴────┐
    │         │
  Tanındı   UNKNOWN
    │         │
    │         ▼
    │   ┌──────────────────┐
    │   │ Intent Classifier│  Tier 1 (LLM)
    │   │ Model Runtime    │
    │   └────────┬─────────┘
    │            │
    └─────┬──────┘
          │
          │  intent: "open_application", target: "firefox"
          ▼
┌────────────────────┐
│   Orchestrator     │
│                    │
│  3. Agent tap      │  registry_find_by_capability("open_application")
│  4. Policy yoxla   │  → ai-policy-engine
└────────┬───────────┘
         │
    ┌────┴────┐
    │         │
 İcazə var  RƏDD
    │         │
    │         ▼
    │   {"status":"error","error_code":"POLICY_DENIED"}
    │
    ▼
┌────────────────────┐
│   Agent            │  /run/ai-desktop-agent.sock
│                    │
│  5. Əmri icra et   │  fork + exec firefox
│  6. Nəticə qaytar  │
└────────┬───────────┘
         │
         │  {"id":1,"status":"success",...}
         ▼
      Client
```

## Agent Lifecycle

```
                    ┌──────────────┐
                    │ UNREGISTERED │  Başlanğıc vəziyyət
                    └──────┬───────┘
                           │ agent_init()
                           ▼
                    ┌──────────────┐
                    │ REGISTERING  │  Orchestrator-a qoşulur
                    └──────┬───────┘
                           │ register mesajı göndərir
              ┌────────────┴────────────┐
              │                         │
         Uğurlu                       Səhv
              │                         │
              ▼                         ▼
     ┌────────────────┐       ┌──────────────┐
     │  REGISTERED    │       │    ERROR     │
     └───────┬────────┘       └──────────────┘
             │ agent_start()
             ▼
     ┌────────────────┐
     │   RUNNING      │  Heartbeat + task qəbulu
     └───────┬────────┘
             │ agent_stop()
             ▼
     ┌────────────────┐
     │   SHUTDOWN     │  Təmiz bağlanma
     └────────────────┘
```

## Maksimum Ölçülər

| Parametr | Dəyər | Tətbiq olunduğu yer |
|----------|-------|---------------------|
| Mesaj ölçüsü | 65536 B | `MAX_MESSAGE_SIZE` |
| Agent sayı | 64 | `MAX_AGENTS` |
| Hər agentə düşən capability | 32 | `MAX_CAPABILITIES` |
| Agent adı uzunluğu | 64 | `MAX_NAME_LEN` |
| Socket yolu uzunluğu | 256 | `MAX_SOCKET_LEN` |
| Sorğu uzunluğu | 2048 | `MAX_QUERY_LEN` (Rule Engine) |
| İntent adı uzunluğu | 128 | `MAX_INTENT_NAME_LEN` |
| Hədəf uzunluğu | 256 | `MAX_TARGET_LEN` |
| Əmr feli uzunluğu | 128 | `MAX_ACTION_LEN` |
| Qoşulma sayı | 256 | `MAX_CONNECTIONS` |
| Keş ölçüsü | 256 | `MAX_CACHE_ENTRIES` (Rule Engine) |
| Regex qaydaları | 1024 | `MAX_REGEX_RULES` |
| Token qaydaları | 1024 | `MAX_TOKENS` |
| Intent cədvəli | 2048 | `MAX_INTENT_ENTRIES` |
| Alias sayı | 1024 | `MAX_ALIASES` |
