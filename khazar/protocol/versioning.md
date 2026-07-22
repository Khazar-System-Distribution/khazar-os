# Versiya Siyasəti

## Semantic Versioning

Bütün komponentlər **SemVer 2.0.0** (`MAJOR.MINOR.PATCH`) istifadə edir.

| Dəyişiklik | Versiya dəyişir | Misal |
|-----------|----------------|-------|
| Backward-compatible bug fix | `PATCH` | `0.1.0` → `0.1.1` |
| Backward-compatible yeni funksiya | `MINOR` | `0.1.1` → `0.2.0` |
| Breaking API dəyişikliyi | `MAJOR` | `0.2.0` → `1.0.0` |

## Breaking Change Nümunələri

Aşağıdakılar **MAJOR** versiya artımı tələb edir:

1. Mesaj formatında sahə silinməsi
2. `type` dəyərinin dəyişməsi (məs. `"register"` → `"agent_register"`)
3. Məcburi sahə əlavə olunması
4. Socket yolunun dəyişməsi
5. Error kodunun silinməsi
6. Timeout dəyərinin azaldılması

Aşağıdakılar **MINOR** versiya artımı tələb edir:

1. Könüllü sahə əlavə olunması
2. Yeni mesaj tipi əlavə olunması
3. Yeni error kodu əlavə olunması
4. Yeni intent tipi əlavə olunması

Aşağıdakılar **PATCH** versiya artımı tələb edir:

1. Bug fix (mesaj formatı dəyişmir)
2. Performans yaxşılaşdırması
3. Sənəd yeniləməsi

## `0.x.x` Qaydaları

`0.MAJOR.MINOR` dövründə (hazırki vəziyyət):

- `MINOR` = breaking change
- `PATCH` = bug fix / yeni funksiya

`1.0.0` stabil buraxılışa qədər API dəyişə bilər.

## Protokol Versiyası

Protokol versiyası **ən yüksək dəstəklənən komponent versiyası ilə eynidir**.

Protokol dəyişəndə `ai-protocol` reposunda `VERSION` faylı yenilənir.
Bütün komponentlər `VERSION` faylındakı versiyanı hədəfləyir.

## Deprecation Policy

1. Deprecated olacaq sahə/API **1 MINOR versiya** əvvəldən elan olunur
2. Deprecation mesajda `"deprecated": true` sahəsi ilə bildirilir
3. Növbəti MAJOR versiyada silinir

```json
{
    "type": "response",
    "old_field": "value",
    "deprecated": true,
    "deprecation_message": "use 'new_field' instead"
}
```

## Uyğunluq Matrisi

| Orchestrator | Rule Engine | SDK | Protokol | Status |
|-------------|-------------|-----|----------|--------|
| ≥0.1.x | ≥0.3.x | ≥0.1.x | v0.1 | ✅ Uyğun |
| <0.1.x | hər hansı | hər hansı | v0.1 | ❌ Uyğun deyil |

## Cari Versiyalar

| Komponent | Versiya |
|-----------|---------|
| `ai-protocol` | `0.1.0` |
| `ai-orchestrator` | `0.1.0` |
| `ai-rule-engine` | `0.3.0` |
| `ai-sdk` | `0.1.0` |
