# Contributing to KhazarOS

Töhfə vermək istədiyiniz üçün təşəkkürlər!

## Başlamazdan Əvvəl

1. **Issue açın** — böyük dəyişikliklər üçün əvvəlcə müzakirə edin
2. **Branch yaradın** — `feature/xxx` və ya `fix/xxx` formatında
3. **Test edin** — `make -C sdk test` bütün testləri keçməlidir

## Repo Qaydaları

- **Dil**: Commit mesajları İngiliscə, kod şərhləri Azərbaycan türkcəsi
- **Stil**: Mövcud C11 kod stilini izləyin (4 boşluq indent, `snake_case` funksiyalar)
- **Test**: Yeni kod üçün test yazın
- **Build**: `make all` 0 error ilə qurulmalıdır

## Branch Adlandırma

```
feature/xxx     — Yeni xüsusiyyət
fix/xxx         — Bug fix
docs/xxx        — Sənəd yeniləməsi
refactor/xxx    — Kod təmizləmə
distro/xxx      — Distro ilə bağlı
```

## Commit Formatı

```
<tip>: <qısa izah>

Ətraflı izah (lazımdırsa).
```

Tiplər: `feat`, `fix`, `docs`, `refactor`, `test`, `build`, `distro`

Nümunə:
```
feat: desktop agent — added close_application support

Uses wmctrl to close windows by title.
```

## Yeni Agent Yazmaq

1. `agents/<name>-agent/` qovluğu yaradın
2. `main.c` yazın (mövcud agentləri nümunə götürün)
3. SDK API-lərini istifadə edin: `#include "sdk/ipc/ipc.h"` etc.
4. Makefile əlavə edin
5. Test edin: `make agents`

## Test

```bash
make -C sdk test           # SDK testləri (22)
make -C components/rule-engine test  # Rule Engine (49)
make -C components/policy-engine test # Policy (7)
make -C components/model-runtime test # Model Runtime (4)
make -C components/intent-classifier test # Classifier (12)
```

## Əlaqə

- GitHub Issues: [github.com/Khazar-System-Distribution/khazar/issues](https://github.com/Khazar-System-Distribution/khazar/issues)
