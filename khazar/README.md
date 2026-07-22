<p align="center">
  <img src="distro/branding/khazar-logo.png" width="160" alt="KhazarOS">
</p>

<h1 align="center">KhazarOS</h1>
<p align="center"><strong>AI-Powered Linux Desktop — Fedora Silverblue + Local AI</strong></p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-blue"></a>
  <img src="https://img.shields.io/badge/language-C11-e94560">
  <img src="https://img.shields.io/badge/build-0_errors-success">
  <img src="https://img.shields.io/badge/tests-94+-brightgreen">
  <img src="https://img.shields.io/badge/arch-x86__64-orange">
  <a href="https://github.com/Khazar-System-Distribution/khazar/releases"><img src="https://img.shields.io/badge/download-ISO-blueviolet"></a>
</p>

<hr>

<h3 align="center">"open firefox" — "turn off wifi" — "volume up" — "update system"</h3>
<p align="center">Control your desktop with natural language. English and Azerbaijani Turkish support. Fully open source. Works offline.</p>

---

## What is KhazarOS?

KhazarOS is an AI-native Linux distribution built on Fedora Silverblue. It replaces manual system configuration with natural language commands — you type (or speak) what you want, and KhazarOS executes it.

Five core principles:

| Principle | How KhazarOS delivers it |
|-----------|--------------------------|
| **Privacy-first** | All AI inference runs locally. No cloud dependency. |
| **Fully open source** | GPLv3. Every line of code is public. |
| **Multilingual** | English and Azerbaijani Turkish. Extensible. |
| **Security by design** | Policy Engine validates every action before execution. |
| **Immutable** | Fedora Silverblue base — atomic updates, rollback safety. |

### Comparison

| Feature | KhazarOS | Windows Copilot | macOS Siri | Stock Fedora |
|---------|----------|----------------|------------|-------------|
| Open source | GPLv3 | Closed | Closed | GPL |
| Offline AI | Local GGUF | Cloud only | Cloud only | None |
| Turkish support | Native | Partial | Partial | Basic |
| Security layer | Policy Engine | None | None | None |
| Immutable OS | rpm-ostree | No | No | Optional |
| Voice control | Planned | Yes | Yes | None |
| Package manager | apt/pacman | winget | brew | dnf |

---

## Quick Demo

```bash
$ kha "open firefox"
  Status: success
  Result: opened firefox

$ kha "turn off wifi"
  Status: success
  Result: WiFi disabled

$ kha "volume up"
  Status: success
  Result: volume +5%

$ kha "update system"
  Status: success
  Result: system packages updated

$ kha shutdown
  Status: error
  Error: POLICY_DENIED — requires policy check

$ kha status
  orchestrator       active
  rule-engine        active
  policy-engine      active
  desktop-agent      active
  package-agent      active
  network-agent      active
  power-agent        active
  audio-agent        active
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        KhazarOS Desktop                          │
│                                                                  │
│  ┌────────────┐          ┌──────────────────────────────────┐   │
│  │   GNOME    │          │      Khazar Daemon (systemd)      │   │
│  │   Shell    │          │                                   │   │
│  │            │          │  ┌─────────┐   ┌──────────────┐  │   │
│  │  Panel AI  │          │  │  Tier 0 │   │   Tier 1     │  │   │
│  │  Icon  ────┼──────────┼─►│  Rule   │   │  Classifier  │  │   │
│  │            │          │  │  Engine │──►│  (LLM)       │  │   │
│  │  Ctrl+     │          │  │ 31 int. │   │  mock+llama  │  │   │
│  │  Space ────┼──────────┼─►└────┬────┘   └──────┬───────┘  │   │
│  │            │          │       │                 │          │   │
│  └────────────┘          │  ┌────▼─────────────────▼──────┐  │   │
│                          │  │     Orchestrator v0.3       │  │   │
│                          │  │  Intent routing + Registry  │  │   │
│                          │  └────┬──────────────┬─────────┘  │   │
│                          │       │              │             │   │
│                          │  ┌────▼────┐   ┌─────▼────────┐  │   │
│                          │  │ Policy  │   │   5 Agents   │  │   │
│                          │  │ Engine  │   │  Desktop     │  │   │
│                          │  │ 13 rule │   │  Package     │  │   │
│                          │  │ ALLOW/  │   │  Network     │  │   │
│                          │  │ DENY    │   │  Power       │  │   │
│                          │  └─────────┘   │  Audio       │  │   │
│                          │                 └──────────────┘  │   │
│                          │  ┌──────────────────────────────┐ │   │
│                          │  │     Model Runtime            │ │   │
│                          │  │  GGUF + llama.cpp             │ │   │
│                          │  └──────────────────────────────┘ │   │
│                          └───────────────────────────────────┘   │
│                                                                  │
│  Base: Fedora Silverblue 40 (rpm-ostree, atomic updates)         │
│  Build: podman -> bootc -> ISO                                   │
└─────────────────────────────────────────────────────────────────┘
```

## Pipeline (5-Stage Flow)

```
Step 1: User Query
    $ kha "open firefox"
    -> JSON -> Unix Socket -> Orchestrator

Step 2: Intent Resolution (Tier 0)
    Rule Engine: 31 intents, 5 stages (cache -> regex -> token -> intent -> alias)
    "open firefox" -> open_application, target=firefox
    UNKNOWN -> Tier 1 (LLM Classifier)

Step 3: Agent Discovery (Registry)
    capability: open_application -> agent: desktop-agent
    socket: /run/khazar/desktop-agent.sock

Step 4: Security Check (Policy Engine)
    agent=desktop-agent, cap=open_application
    -> rule: [rule.allow_desktop] priority=10 -> ALLOW

Step 5: Execution (Agent Dispatch)
    Orchestrator -> desktop-agent socket -> fork+exec firefox
    -> Response: {"status":"success","message":"opened firefox"}
```

## Supported Commands (31 Intents)

### Application Control
| Command | Intent | Agent |
|---------|--------|-------|
| `open firefox` | open_application | desktop-agent |
| `close telegram` | close_application | desktop-agent |
| `maximize window` | window_maximize | desktop-agent |

### Package Management
| Command | Intent | Agent |
|---------|--------|-------|
| `install steam` | install_package | package-agent |
| `remove vlc` | remove_package | package-agent |
| `search firefox` | search_package | package-agent |
| `update system` | system_update | package-agent |

### Network
| Command | Intent | Agent |
|---------|--------|-------|
| `wifi on` | network_enable | network-agent |
| `wifi off` | network_disable | network-agent |
| `network status` | network_status | network-agent |

### Power
| Command | Intent | Agent |
|---------|--------|-------|
| `shutdown` | system_shutdown | power-agent |
| `reboot` | system_reboot | power-agent |
| `sleep` | system_suspend | power-agent |
| `lock screen` | system_lock | power-agent |
| `logout` | system_logout | power-agent |

### Audio
| Command | Intent | Agent |
|---------|--------|-------|
| `volume up` | volume_up | audio-agent |
| `volume down` | volume_down | audio-agent |
| `mute` | volume_mute | audio-agent |

### Display + Files + Other
| Command | Intent |
|---------|--------|
| `screenshot` | screenshot |
| `open documents` | file_open |
| `search file config` | file_search |
| `brightness up` | brightness_up |

> **31 intents total** — all resolved at Tier 0 with sub-millisecond latency.

---

## Installation

### Option A: Download Pre-built ISO (GitHub Releases)

Download the latest ISO from [GitHub Releases](https://github.com/Khazar-System-Distribution/khazar-distro/releases).

```bash
# Write to USB
sudo dd if=khazaros.iso of=/dev/sdX bs=4M status=progress

# Test in QEMU
qemu-system-x86_64 -m 4096 -enable-kvm -hda khazaros.iso
```

### Option B: Build ISO Yourself (Fedora 40+)

Requires Fedora 40+. Produces a complete 3-6 GB bootable ISO.

```bash
git clone https://github.com/Khazar-System-Distribution/khazar-distro
cd khazar-distro
bash build-iso.sh
# → khazaros-0.1.0-x86_64.iso
```

One command. Takes ~15 minutes (downloads Fedora Silverblue base once, then caches).

### Option B: RPM Package (Existing Fedora)

```bash
rpmbuild -ba distro/rpm/khazar.spec
rpm-ostree install khazar-0.1.0-1.fc40.x86_64.rpm
systemctl reboot
systemctl enable khazar.target --now
```

### Option C: Build from Source

```bash
git clone https://github.com/Khazar-System-Distribution/khazar
cd khazar
make all                              # 0 errors, 94+ tests pass
sudo bash distro/install.sh
systemctl enable khazar.target --now
kha "open firefox"
```

---

## Developer Documentation

### Repository Structure

```
khazar/                                 # Monorepo — 200+ files, 19K+ lines
├── sdk/                                # Agent SDK (IPC, logger, events)
│   ├── include/common.h                # Shared types and constants
│   ├── ipc/                            # Unix socket client+server (epoll)
│   ├── agent/                          # Agent lifecycle (register, heartbeat)
│   └── events/                         # In-process pub/sub
├── components/                         # Core services
│   ├── orchestrator/                   # Tier 0+1 router (v0.3)
│   │   ├── registry/                   # Agent registration + capability lookup
│   │   ├── rule_client/                # Rule Engine IPC client
│   │   ├── policy_client/              # Policy Engine IPC client
│   │   ├── agent_client/               # Agent dispatch (socket connect+send)
│   │   └── intent_client/              # Tier 1 classifier client
│   ├── rule-engine/                    # Deterministic intent resolver (49 tests)
│   │   ├── cache/                      # LRU cache (O(1), 256 entries)
│   │   ├── matcher/                    # POSIX regex matching (1024 rules)
│   │   ├── tokens/                     # Keyword lookup + scoring
│   │   ├── intent/                     # Exact intent table (2048 entries)
│   │   ├── alias/                      # Synonym resolution (1024 aliases)
│   │   └── fuzzy/                      # Levenshtein fuzzy matching
│   ├── policy-engine/                  # Security policy (13 rules, fnmatch)
│   ├── model-runtime/                  # LLM inference server
│   │   └── inference/                  # Mock backend + llama.cpp stub
│   └── intent-classifier/              # Tier 1 LLM fallback + 32 keywords
├── agents/                             # 5 agents
│   ├── desktop-agent/                  # fork+exec application launcher
│   ├── package-agent/                  # apt/pacman package management
│   ├── network-agent/                  # nmcli WiFi/ethernet control
│   ├── power-agent/                    # systemctl/loginctl power control
│   └── audio-agent/                    # pactl audio control
├── protocol/                           # IPC contracts (JSON Schema, RPC)
├── distro/                             # Distribution build system
│   ├── bootc/Containerfile             # OS image definition
│   ├── gnome/                          # GNOME customization
│   │   ├── theme/Khazar-dark/          # Shell + GTK4 CSS themes
│   │   ├── extensions/                 # Panel icon + Ctrl+Space
│   │   └── settings/                   # GSettings overrides
│   ├── branding/                       # GRUB, Plymouth, GDM, icons
│   ├── systemd/                        # 10 systemd units + khazar.target
│   ├── iso/                            # ISO builder
│   └── cli/kha                         # User-facing CLI tool
└── .github/workflows/build.yml         # CI pipeline
```

### Build Status

| Component | Files | Build | Tests |
|-----------|-------|-------|-------|
| `sdk` | 14 | 0 errors | 22/22 |
| `orchestrator` | 20 | 0 errors | -- |
| `rule-engine` | 26 | 0 warnings | 49/49 |
| `policy-engine` | 11 | 0 errors | 7/7 |
| `model-runtime` | 11 | 0 errors | 4/4 |
| `intent-classifier` | 7 | 0 errors | 12/12 |
| `desktop-agent` | 4 | 0 errors | -- |
| `package-agent` | 3 | 0 errors | -- |
| `network-agent` | 3 | 0 errors | -- |
| `power-agent` | 3 | 0 errors | -- |
| `audio-agent` | 3 | 0 errors | -- |
| **TOTAL** | **105** | **0 errors** | **94/94** |

### Build Commands

```bash
make all              # Build all 11 components
make sdk              # SDK only
make components       # Core services only
make agents           # Agents only
make -C sdk test      # SDK tests (22)
make install          # System-wide install
```

### SDK API Reference

```c
#include "sdk/include/common.h"      // request_t, response_t, agent_info_t
#include "sdk/logger/logger.h"       // log_info(), log_error(), log_fatal()
#include "sdk/ipc/ipc.h"             // ipc_server_init(), ipc_server_send()
#include "sdk/protocol/protocol.h"   // protocol_build_response(), protocol_parse_request()
```

**Writing an Agent (5-minute tutorial):**

```c
#define _GNU_SOURCE
#include "sdk/include/common.h"
#include "sdk/logger/logger.h"
#include "sdk/ipc/ipc.h"
#include <stdio.h>
#include <signal.h>

static volatile bool running = true;

static void handler(int client_fd, const char *data, size_t len, void *ctx) {
    (void)ctx; (void)len;
    char out[4096];

    // Parse request from JSON
    request_t req = {0};

    // Execute the command
    system("firefox");

    // Respond
    snprintf(out, sizeof(out),
        "{\"id\":%llu,\"status\":\"success\","
        "\"payload\":{\"message\":\"done\"}}",
        (unsigned long long)req.id);
    ipc_server_send(client_fd, out, strlen(out));
}

int main(void) {
    logger_init(NULL, LOG_INFO);

    // Start IPC server
    ipc_server_t *srv = ipc_server_init("/run/my-agent.sock", 64);
    log_info("agent", "listening on /run/my-agent.sock");

    ipc_server_start(srv, handler, NULL);
    ipc_server_cleanup(srv);
    logger_cleanup();
    return 0;
}
```

Compile:
```bash
gcc -std=c11 -O2 my-agent.c \
    -I/path/to/khazar/sdk/include -I/path/to/khazar/sdk \
    -L/path/to/khazar/sdk -lai-sdk -pthread -o my-agent
```

---

## GNOME Customization

KhazarOS ships with a complete GNOME visual identity:

| Element | Stock GNOME | KhazarOS |
|---------|------------|----------|
| Shell theme | Adwaita | Khazar Dark (navy + red) |
| GTK4 theme | Adwaita | Khazar Dark |
| Icons | Adwaita | Papirus-Dark |
| Panel | Default | AI Assistant icon + indicator |
| Shortcut | None | Ctrl+Space (command input) |
| Wallpaper | Fedora default | KhazarOS dark abstract |
| Boot screen | Fedora Plymouth | KhazarOS (red logo pulse) |
| GRUB | Fedora | KhazarOS dark theme |
| Login (GDM) | Fedora | KhazarOS (dark + red accent) |
| Font | Cantarell | DejaVu Sans + JetBrains Mono |
| Color scheme | Light | Dark (prefer-dark) |

**Color palette:**

| Color | Hex | Usage |
|-------|-----|-------|
| Dark navy | `#1a1a2e` | Primary background |
| Medium navy | `#16213e` | Panel, headerbar |
| Deep blue | `#0f3460` | Cards, sidebars |
| Khazar red | `#e94560` | Accent, buttons, selections |
| Light text | `#e0e0e0` | Primary text |
| Dim text | `#8888aa` | Secondary text |

---

## Testing

See [TESTING.md](TESTING.md) for detailed instructions on testing KhazarOS in:
- QEMU/KVM (fastest)
- VirtualBox (free, cross-platform)
- Physical hardware (USB boot)

---

## Contributing

KhazarOS is open source and welcomes contributions. See [CONTRIBUTING.md](CONTRIBUTING.md).

**Quick start:**

1. Fork the repository
2. Create a branch: `git checkout -b feature/new-agent`
3. Make changes and test: `make -C sdk test`
4. Commit: `git commit -m "feat: description"`
5. Push and open a Pull Request

### Roadmap

| Version | Target |
|---------|--------|
| `0.1.0` | SDK + 5 components + 5 agents + Policy + GNOME theme |
| `0.2.0` | Flatpak agent, GNOME 47, notification center |
| `0.3.0` | Voice recognition (Whisper.cpp), Azerbaijani LM |
| `0.5.0` | ARM64 (Raspberry Pi 5), power optimization |
| `1.0.0` | Stable release, LTS support |

---

## License

[GNU General Public License v3.0](LICENSE)

Copyright 2025 Khazar System Distribution
