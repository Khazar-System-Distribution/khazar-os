# AI Policy Engine

Khazar AI platformasının təhlükəsizlik qatıdır. Hər agent əməliyyatını
icra edilməzdən əvvəl yoxlayır.

## Prinsip

```
Orchestrator → Policy Engine → Agent
                  │
            ┌─────┴─────┐
          ALLOW        DENY
```

Policy Engine olmadan heç bir agent əməliyyat icra etməməlidir.

## Pipeline

```
1. Orchestrator intent alır (məs: "firefox aç" → open_application)
2. Orchestrator agent tapır (registry_find_by_capability)
3. Orchestrator Policy Engine-ə sorğu göndərir
4. Policy Engine siyasətləri yoxlayır
5. ALLOW → agent əmri icra edir
   DENY  → Orchestrator səhv qaytarır
```

## IPC Protocol

Socket: `/run/ai-policy-engine.sock`

**Policy Check Request:**
```json
{
    "type": "policy_check",
    "id": 1,
    "agent": "desktop-agent",
    "capability": "open_application",
    "target": "firefox"
}
```

**Policy Response (ALLOW):**
```json
{
    "id": 1,
    "type": "policy_response",
    "action": "allow",
    "reason": "application opening is allowed",
    "matched_rule": "cap=open_application agent=*"
}
```

**Policy Response (DENY):**
```json
{
    "id": 2,
    "type": "policy_response",
    "action": "deny",
    "reason": "system shutdown denied by policy",
    "matched_rule": "cap=system_shutdown agent=*"
}
```

## Siyasət Formatı (TOML)

```toml
[rule.rule_name]
capability = "open_application"   # hansı capability?
agent = "*"                       # hansı agent? (* = hamısı)
action = "allow"                  # allow | deny | ask_user
priority = 10                     # yüksək = daha önəmli
require_root = false              # root tələb olunur?
match_type = "exact"             # exact | pattern | all
message = "..."                   # cavab mesajı
```

## Prioritet Sistemi

Eyni capability + agent üçün çoxlu qayda ola bilər.
**Ən yüksək prioritetli qayda** qalib gəlir.

```
[rule.allow_all]      priority=5   → allow  (default)
[rule.deny_shutdown]  priority=50  → deny   (override)
```

## Modullar

| Modul | Fayl | Funksiya |
|-------|------|----------|
| `config` | `config/` | TOML konfiqurasiya yükləmə |
| `policy` | `policy/` | Siyasət yükləmə + yoxlama + fnmatch |
| `protocol` | `protocol/` | Policy-specific mesaj formatı |

## Build

```bash
make
```

## Test

```bash
make test
```

```
--- Policy Engine Unit Tests ---
PASS: policy_init
PASS: policy_add_and_check (allow)
PASS: policy_add_and_check (deny)
PASS: policy_priority (higher wins)
PASS: protocol_parse_policy_check
PASS: protocol_build_response
PASS: protocol_build_response_deny
All Policy Engine tests passed
```

## Quraşdırma

```bash
sudo make install
ai-policy-engine /etc/ai-policy-engine.toml
```

## Asılılıqlar

- C11 compiler (gcc/clang)
- `libai-sdk.a` (logger, IPC, common types)
- POSIX regex (`fnmatch.h`)

## Əlaqəli Repositorilər

| Repo | Əlaqə |
|------|-------|
| `ai-orchestrator` | Policy Engine-i çağırır, nəticəyə əsasən agentə yönləndirir |
| `ai-rule-engine` | Intent təyini (policy yoxlamasından əvvəl) |
| `ai-sdk` | Logger + IPC kitabxanası |
| `ai-protocol` | Ortaq protokol sənədləri |
