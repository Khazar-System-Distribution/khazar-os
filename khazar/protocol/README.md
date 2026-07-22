# AI Protocol

Khazar AI platformasında bütün komponentlər arası IPC protokolu üçün
**tək həqiqət mənbəyi** (single source of truth).

Heç bir kod yoxdur — yalnız sənədlər, JSON sxemlər və konvensiyalar.

## İçindəkilər

| Fayl | Məzmun |
|------|--------|
| [`protocol.md`](./protocol.md) | Bütün mesaj formatları, IPC arxitekturası, request lifecycle |
| [`rpc.md`](./rpc.md) | Request/response cütləşməsi, timeout, retry, ping/pong |
| [`capability-schema.json`](./capability-schema.json) | Agent qeydiyyatı və capability modeli |
| [`action-schema.json`](./action-schema.json) | Intent/action formatı |
| [`error-codes.md`](./error-codes.md) | Standart səhv kodları cədvəli |
| [`versioning.md`](./versioning.md) | Semantic versiyalama siyasəti |

## İstifadə edən komponentlər

```
ai-protocol
    ↑
    ├── ai-orchestrator     — Tier 0 router, agent registry
    ├── ai-rule-engine      — Deterministik intent classifier
    ├── ai-sdk             — Agent SDK (bu protokolu implementə edir)
    ├── ai-policy-engine   — Təhlükəsizlik siyasəti yoxlaması
    ├── ai-model-runtime   — LLM inference servisi
    ├── ai-intent-classifier — Tier 1 LLM classifier
    └── bütün agentlər     — desktop, package, network, ...
```

## Prinsiplər

1. **JSON over Unix Domain Sockets** — bütün IPC rabitəsi
2. **Newline-delimited** — hər mesaj bir sətir, maksimum 64 KB
3. **Asinxron RPC** — `id` ilə request/response cütləşməsi
4. **Capability-based routing** — agentlər `what they can do` ilə qeydiyyatdan keçir
5. **Tiered inference** — Tier 0 (deterministik) → Tier 1 (LLM)
