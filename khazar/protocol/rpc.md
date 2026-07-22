# RPC Konvensiyası

## Request/Response Cütləşməsi

Bütün RPC sorğuları `id` sahəsi ilə cütləşir.
Client göndərir, server eyni `id` ilə cavab verir.

```
Client:  {"id": 42, "type": "request", "query": "firefox aç"}
Server:  {"id": 42, "status": "success", ...}
```

`id` unikal olmalıdır (monoton artan uint64 tövsiyə olunur).

## Timeout

| Əməliyyat | Timeout | İzah |
|-----------|---------|------|
| Request | 3000 ms | Agent cavab verməlidir |
| Heartbeat interval | 5000 ms | Agent hər 5 saniyədən bir siqnal göndərir |
| Heartbeat timeout | 15000 ms | 15 saniyə səssizlik = agent ölü |
| Qoşulma | 3000 ms | Socket bağlantısı qurma |

Timeout keçərsə, Orchestrator `ERR_TIMEOUT` qaytarır.

## Ping/Pong

Bağlantı sağlamlığını yoxlamaq üçün:

```
Client → Server:  {"type": "ping"}
Server → Client:  {"status": "alive"}
```

Ping-ə cavab verilməzsə, bağlantı kəsilmiş sayılır.

## Retry Siyasəti

| Ssenari | Davranış |
|---------|----------|
| Agent cavab vermir | Orchestrator `ERR_AGENT_TIMEOUT` qaytarır, retry yoxdur |
| Agent bağlantısı kəsilir | Orchestrator növbəti uyğun agentə yönləndirir |
| Rule Engine cavab vermir | Orchestrator Tier 1-ə keçir (əgər mövcuddursa) |
| Qoşulma kəsilir | Client yenidən qoşula bilər, yeni `id` ilə |

## Paralel Sorğular

Eyni socket üzərindən paralel sorğu göndərmək olar.
Cavablar `id` ilə cütləşdiyi üçün sıra pozula bilər.

```
Client göndərir: {"id": 1, ...}  {"id": 2, ...}  {"id": 3, ...}
Server cavab:           {"id": 2, ...}  {"id": 1, ...}  {"id": 3, ...}
                              ↑ sıra pozula bilər, id ilə həll olunur
```

## Asinxron Hadisələr

Server (və ya agent) sorğusuz da hadisə göndərə bilər:

```json
{
    "type": "event",
    "event_type": "system.shutdown",
    "data": {"reason": "user_requested"}
}
```

## Socket Qaydaları

1. **Bir socket = bir servis.** Hər servis (orchestrator, rule-engine, agent) öz socket-ində dinləyir.
2. **Qısa ömürlü bağlantılar.** Client bağlanır, sorğu göndərir, cavab alır, bağlanır.
3. **Uzun ömürlü bağlantılar.** Agent Orchestrator-a bağlı qalır, heartbeat göndərir.
4. **Permissiyalar.** Socket faylları 0660 (`rw-rw----`) icazə ilə yaradılır.

## Mesaj Aşkarlanması

Mesaj tipi `type` sahəsinə görə müəyyən olunur.
Prioritet sırası (ən spesifikdən ümumiyə):

1. `"type":"ping"` → MSG_PING
2. `"type":"pong"` → MSG_PONG
3. `"type":"heartbeat"` → MSG_HEARTBEAT
4. `"type":"event"` → MSG_EVENT
5. `"type":"register"` → MSG_REGISTER
6. `"type":"unregister"` → MSG_UNREGISTER
7. `"type":"response"` → MSG_RESPONSE
8. `"type":"version"` → MSG_VERSION
9. `"type":"metrics"` → MSG_METRICS
10. `"type":"add_rule"` → MSG_ADD_RULE
11. `"type":"reload"` → MSG_RELOAD
12. `"type":"request"` **və ya** `"query"` sahəsi varsa → MSG_REQUEST
13. Heç biri deyilsə → MSG_UNKNOWN
