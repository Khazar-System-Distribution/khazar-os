# AI Orchestrator Documentation v0.1

## Məqsəd

AI Orchestrator sistemin mərkəzi koordinasiya komponentidir.

Bütün istifadəçi sorğuları ilk olaraq bu daemon tərəfindən qəbul edilir və emal olunur.

Orchestrator heç vaxt əməliyyatları özü icra etmir.

Onun əsas vəzifəsi:

* Sorğunu qəbul etmək
* Sorğunu analiz etmək
* Uyğun agenti seçmək
* Təhlükəsizlik yoxlamalarını aparmaq
* Cavabı istifadəçiyə geri qaytarmaq

Bu komponent sistemin PID 1 deyil, lakin istifadəçi səviyyəsindəki bütün AI əməliyyatlarının faktiki init sistemi rolunu oynayır.

---

# Dizayn Prinsipləri

## 1. Single Responsibility Principle

Orchestrator yalnız koordinasiya ilə məşğul olur.

Qadağandır:

* Paket quraşdırmaq
* Wifi idarə etmək
* Tətbiq açmaq
* Shell əmri icra etmək
* DBus ilə birbaşa danışmaq

Bu işlərin hamısı agentlər tərəfindən həyata keçirilir.

---

## 2. Zero Trust Architecture

Heç bir agentə tam etibar edilmir.

Hər request aşağıdakı mərhələlərdən keçir:

User

↓

Orchestrator

↓

Rule Engine

↓

Intent Classifier

↓

Policy Engine

↓

Agent

↓

Response

---

## 3. Local First Design

Bütün inference lokal həyata keçirilir.

Bulud servislərinə dependency yoxdur.

İnternet bağlantısı olmadan tam işləməlidir.

---

## 4. Minimal Memory Usage

Idle vəziyyətdə hədəf:

50 MB RAM-dan aşağı.

Yüksək yüklənmə zamanı:

100 MB RAM-dan aşağı.

Inference komponentləri ayrıca process olaraq işlədiyindən əsas daemon böyük model yükü daşımır.

---

# Repository Strukturu

```text
ai-orchestrator/

├── src/
│
├── include/
│
├── ipc/
│
├── router/
│
├── protocol/
│
├── scheduler/
│
├── session/
│
├── registry/
│
├── config/
│
├── logger/
│
├── metrics/
│
├── tests/
│
├── docs/
│
└── main.c
```

---

# Modul Təsvirləri

## ipc/

Unix Domain Socket server implementasiyası.

Funksiyalar:

* socket creation
* socket cleanup
* connection accept
* read/write operations
* timeout handling
* reconnect support

Socket yolu:

```text
/run/ai-orchestrator.sock
```

Socket permission:

```text
0660
```

Owner:

```text
root:ai
```

---

## router/

Request routing sistemi.

Məsuliyyətləri:

* request parsing
* request dispatching
* target agent selection
* response aggregation

Nümunə:

Input:

```json
{
    "query":"firefox aç"
}
```

Output:

```text
desktop-agent
```

---

## protocol/

JSON RPC protokol implementasiyası.

Mesaj formatı:

Request:

```json
{
    "id":1,
    "type":"request",
    "payload":{
        "query":"firefox aç"
    }
}
```

Response:

```json
{
    "id":1,
    "status":"success",
    "payload":{
        "message":"application started"
    }
}
```

Error:

```json
{
    "id":1,
    "status":"error",
    "error_code":"AGENT_UNAVAILABLE"
}
```

---

## scheduler/

Agent request scheduling sistemi.

Məsuliyyətləri:

* queue management
* timeout management
* retry policy
* prioritization

Priority səviyyələri:

0 -> critical

1 -> high

2 -> normal

3 -> background

---

## session/

İstifadəçi sessiyalarının idarə olunması.

Məsuliyyətləri:

* context storage
* session timeout
* history tracking
* state management

Məsələn:

User:

```text
firefox aç
```

User:

```text
onu bağla
```

"onu" referansının Firefox olduğunu session manager müəyyən edir.

---

## registry/

Agent qeydiyyat sistemi.

Hər agent startup zamanı özünü qeydiyyatdan keçirir.

Nümunə:

```json
{
    "name":"desktop-agent",
    "version":"1.0",
    "socket":"/run/desktop-agent.sock",
    "capabilities":[
        "open_application",
        "close_application",
        "notifications"
    ]
}
```

---

## logger/

Mərkəzləşdirilmiş log sistemi.

Log səviyyələri:

* TRACE
* DEBUG
* INFO
* WARN
* ERROR
* FATAL

Log formatı:

```text
2026-07-14T21:15:31Z INFO desktop-agent request accepted
```

---

## metrics/

Performans monitorinq sistemi.

Toplanacaq metriklər:

* request latency
* average response time
* agent failures
* queue size
* memory usage
* cpu usage

---

## config/

Konfiqurasiya faylları.

Format:

TOML

Nümunə:

```toml
socket_path="/run/ai-orchestrator.sock"
max_connections=256
request_timeout_ms=3000
enable_metrics=true
```

---

# IPC Dizaynı

Kommunikasiya:

Unix Domain Socket

Format:

JSON RPC

Maksimum mesaj ölçüsü:

```text
64 KB
```

Maksimum əlaqə sayı:

```text
256
```

Maksimum request timeout:

```text
3000 ms
```

---

# Request Lifecycle

## Stage 1

Request qəbul edilir.

```json
{
    "query":"firefox aç"
}
```

---

## Stage 2

Rule Engine yoxlanılır.

Əgər uyğunluq tapılırsa:

```json
{
    "action":"open_application",
    "application":"firefox"
}
```

yaradılır.

---

## Stage 3

Policy Engine icazələri yoxlayır.

---

## Stage 4

Desktop Agent seçilir.

---

## Stage 5

Desktop Agent əməliyyatı icra edir.

---

## Stage 6

Nəticə istifadəçiyə qaytarılır.

---

# Thread Model

Main Thread:

* accept()
* event loop

Worker Threads:

* request parsing
* routing
* validation

Agent communication ayrıca worker thread pool vasitəsilə həyata keçirilir.

İlkin hədəf:

```text
4 worker thread
```

---

# Failure Recovery

Əgər agent cavab vermirsə:

```text
AGENT_TIMEOUT
```

Əgər socket bağlanırsa:

```text
AGENT_DISCONNECTED
```

Əgər validator requesti bloklayırsa:

```text
POLICY_DENIED
```

---

# İlk Milestone

MVP hədəfi:

Client:

```bash
echo '{"type":"ping"}' \
| socat - UNIX-CONNECT:/run/ai-orchestrator.sock
```

Response:

```json
{
    "status":"alive"
}
```

---

# İkinci Milestone

Client:

```json
{
    "query":"firefox aç"
}
```

Response:

```json
{
    "status":"success"
}
```

Bu mərhələdə desktop-agent hələ real Firefox açmaya bilər.

Sadəcə mock response qaytarılması kifayətdir.

---

# Üçüncü Milestone

Desktop agent ilə real IPC inteqrasiyası.

Bu nöqtədən sonra sistem artıq praktik istifadə edilə bilən vəziyyətə yaxınlaşmağa başlayır.

