# AI Rule Engine

Khazar System Distribution AI platformasının Tier 0 inference sistemidir. LLM istifadə etmədən sürətli əmr tanıma təmin edir.

## Prinsip

Rule Engine **heç vaxt** LLM çağırmır. Bütün əmrlər deterministik qaydalarla tanınır:

```
İstifadəçi əmri → Tier 0 (Rule Engine) → Intent → Orchestrator → Agent
                       ↑
                  LLM istifadə olunmur
```

Əgər Tier 0 əmri tanıya bilmirsə, nəticə `UNKNOWN` qaytarılır və Orchestrator Tier 1-ə (ai-intent-classifier, LLM) yönləndirir.

## Tier 0 Pipeline

```
Query daxil olur
     │
     ▼
┌─────────────┐
│ 1. Cache    │  ← ən sürətli yol (O(1) hash lookup)
│    lookup   │     son 256 əmr yadda saxlanılır
└──────┬──────┘
       │ tapılmadı
       ▼
┌─────────────┐
│ 2. Regex    │  ← pattern matching (POSIX regex)
│    match    │     məs: "^(.*) aç$" → open_application
└──────┬──────┘
       │ tapılmadı
       ▼
┌─────────────┐
│ 3. Token    │  ← açar söz lookup + scoring
│    lookup   │     hər tokenə uyğun intent tapılır
└──────┬──────┘
       │ tapılmadı
       ▼
┌─────────────┐
│ 4. Intent   │  ← birbaşa intent cədvəli
│    table    │     tam əmr → intent mapping
└──────┬──────┘
       │ tapılmadı
       ▼
┌─────────────┐
│ 5. Alias    │  ← alternativ adlar
│    resolve  │     "sistemi yenilə" = "update system"
└──────┬──────┘
       │
       ▼
   UNKNOWN → Tier 1-ə yönləndir
```

## Modullar

| # | Modul | Funksiya |
|---|-------|----------|
| 1 | cache/ | LRU command cache (O(1) lookup) |
| 2 | matcher/ | POSIX regex pattern matching |
| 3 | tokens/ | Token lookup + similarity scoring |
| 4 | intent/ | Intent table (tam əmr mapping) |
| 5 | alias/ | Alias support + synonym resolution |
| 6 | ipc/ | Unix Domain Socket server (epoll) |
| 7 | protocol/ | JSON-RPC mesaj formatı |
| 8 | logger/ | Thread-safe logging (6 səviyyə) |
| 9 | config/ | TOML konfiqurasiya |

## Nümunələr

| Əmr | Regex | Token | Intent | Nəticə |
|-----|-------|-------|--------|--------|
| `firefox aç` | `^(.*) aç$` | aç→open | firefox | open_application:firefox |
| `wifi söndür` | `^wifi (.*)$` | söndür→disable | wifi | network_disable:wifi |
| `sistemi yenilə` | `^sistemi yenilə$` | yenilə→update | sistem | system_update |
| `telegram aç` | `^(.*) aç$` | aç→open | telegram | open_application:telegram |
| `steam quraşdır` | `^(.*) quraşdır$` | quraşdır→install | steam | install_package:steam |

## IPC Protocol

Socket: `/run/ai-rule-engine.sock`

Request:
```json
{"id":1,"type":"request","query":"firefox aç"}
```

Response (tanındı):
```json
{"id":1,"type":"response","intent":"open_application","target":"firefox","action":"aç","confidence":1.0}
```

Response (tanınmadı):
```json
{"id":1,"type":"response","intent":"unknown","confidence":0.0}
```

## Build

```bash
make
```

## Test

```bash
make test
echo '{"id":1,"type":"request","query":"firefox aç"}' | socat - UNIX-CONNECT:/run/ai-rule-engine.sock
```

## Quraşdırma

```bash
sudo make install
ai-rule-engine /etc/ai-rule-engine.toml
```

## Asılılıqlar

- C11 compiler (gcc/clang)
- POSIX threads
- POSIX regex (regex.h)
- Linux kernel (epoll)

## Əlaqəli Repositorilər

| Repo | Əlaqə |
|------|-------|
| ai-orchestrator | Rule Engine-i çağırır, nəticəni agentə yönləndirir |
| ai-intent-classifier | Tier 1 (LLM) — Rule Engine tanımadıqda işə düşür |
| ai-policy-engine | Intent nəticəsini yoxlayır |
