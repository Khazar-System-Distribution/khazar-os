# Standart Səhv Kodları

Bütün komponentlər eyni səhv kodlarından istifadə edir.
Səhv kodları `error_code_str()` funksiyası ilə string-ə çevrilir.

## Səhv Cədvəli

| Kod | Enum | Wire String | HTTP Analoqu | İzah |
|-----|------|-------------|-------------|------|
| 0 | `ERR_NONE` | `"NONE"` | 200 | Səhv yoxdur |
| 1 | `ERR_AGENT_TIMEOUT` | `"AGENT_TIMEOUT"` | 504 | Agent 3000ms ərzində cavab vermədi |
| 2 | `ERR_AGENT_DISCONNECTED` | `"AGENT_DISCONNECTED"` | 503 | Agent gözlənilmədən bağlantını kəsdi |
| 3 | `ERR_POLICY_DENIED` | `"POLICY_DENIED"` | 403 | Policy Engine əməliyyatı rədd etdi |
| 4 | `ERR_INTERNAL` | `"INTERNAL_ERROR"` | 500 | Daxili server xətası |
| 5 | `ERR_INVALID_REQUEST` | `"INVALID_REQUEST"` | 400 | Yanlış formatda sorğu |
| 6 | `ERR_AGENT_UNAVAILABLE` | `"AGENT_UNAVAILABLE"` | 404 | Tələb olunan capability üçün agent tapılmadı |
| 7 | `ERR_SYSTEM` | `"SYSTEM_ERROR"` | 500 | Sistem səviyyəli xəta (resurs, yaddaş) |
| 8 | `ERR_NOT_REGISTERED` | `"NOT_REGISTERED"` | 401 | Qeydiyyatsız agent heartbeat göndərir |
| 9 | `ERR_REGISTRATION_FAILED` | `"REGISTRATION_FAILED"` | 400 | Qeydiyyat mesajı rədd edildi |
| 10 | `ERR_CONNECTION_FAILED` | `"CONNECTION_FAILED"` | 502 | Socket-ə qoşulmaq mümkün olmadı |
| 11 | `ERR_PROTOCOL_ERROR` | `"PROTOCOL_ERROR"` | 400 | Protokol pozuntusu |
| 12 | `ERR_TIMEOUT` | `"TIMEOUT"` | 504 | Ümumi timeout |

## Səhv Cavab Formatı

### SDK Formatı (ətraflı)

```json
{
    "id": 42,
    "status": "error",
    "error_code": "AGENT_UNAVAILABLE",
    "payload": {
        "message": "no agent registered for capability 'open_application'"
    }
}
```

### Rule Engine Formatı (qısa)

```json
{
    "id": 42,
    "type": "error",
    "error_code": "INVALID_REQUEST"
}
```

## Ssenarilər

### Agent Tapılmadı

```
Sorğu:     {"id":1,"type":"request","query":"blender aç"}
Intent:    open_application, target=blender
Agent:     registry_find_by_capability("open_application") → NULL
Cavab:     {"id":1,"status":"error","error_code":"AGENT_UNAVAILABLE",...}
```

### Policy Rədd Etdi

```
Sorğu:     {"id":2,"type":"request","query":"sistemi söndür"}
Intent:    system_shutdown
Policy:    policy_check("system_shutdown") → DENIED (root tələb olunur)
Cavab:     {"id":2,"status":"error","error_code":"POLICY_DENIED",...}
```

### Agent Timeout

```
Sorğu:     {"id":3,"type":"request","query":"wifi aç"}
Agentə göndərildi:  {"id":3,...}
3000 ms keçdi, cavab yoxdur
Cavab:     {"id":3,"status":"error","error_code":"AGENT_TIMEOUT","payload":{"message":"agent did not respond within 3000ms"}}
```

### Bağlantı Kəsildi

```
Sorğu:     {"id":4,"type":"request","query":"səs artır"}
Agent tapıldı, socket-ə qoşulmağa cəhd
connect() → -1 (connection refused)
Cavab:     {"id":4,"status":"error","error_code":"CONNECTION_FAILED",...}
```
