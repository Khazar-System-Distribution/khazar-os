<p align="center">
  <img src="logo.png" width="180" alt="KhazarOS">
</p>

<h1 align="center">KhazarOS</h1>
<h3 align="center">AI-Powered Fedora Silverblue Downstream</h3>

<p align="center">
  <a href="https://github.com/Khazar-System-Distribution/khazar-os/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-blue"></a>
  <img src="https://img.shields.io/badge/base-Fedora%20Silverblue%2040-blueviolet">
  <img src="https://img.shields.io/badge/build-Universal%20Blue-orange">
  <img src="https://img.shields.io/badge/arch-x86__64-red">
</p>

<p align="center">
  <b>Control your desktop with natural language.</b><br>
  <sub>"open firefox" · "turn off wifi" · "volume up" · "update system"</sub>
</p>

---

## What is KhazarOS?

KhazarOS is a **Fedora Silverblue downstream** — an immutable, atomic desktop OS with a built-in AI platform.

It replaces manual system configuration with natural language commands. Type what you want, KhazarOS executes it.

| | Fedora Silverblue | **KhazarOS** |
|---|---|---|
| Base | Fedora 40 | Fedora 40 (identical kernel + packages) |
| Updates | rpm-ostree (atomic) | rpm-ostree (atomic) |
| Desktop | GNOME 46 | GNOME 46 + Khazar Dark theme |
| System control | Terminal + GUI | `kha "open firefox"` |
| AI | None | 11 systemd daemons, 31 intents |
| Security | Default | Policy Engine (allow/deny rules) |
| Offline | Yes | Yes (local AI, no cloud) |

## Install

### Rebase from Fedora Silverblue 40

```bash
sudo rpm-ostree rebase ostree-unverified-registry:ghcr.io/khazar-system-distribution/khazaros:latest
systemctl reboot
kha "open firefox"
```

### ISO

Download from [Releases](https://github.com/Khazar-System-Distribution/khazar-os/releases).

```bash
sudo dd if=khazaros-latest-x86_64.iso of=/dev/sdX bs=4M status=progress
```

## Usage

```bash
# Applications
kha "open firefox"
kha "close telegram"

# Network
kha "turn on wifi"
kha "network status"

# Audio
kha "volume up"
kha "mute"

# System
kha "update system"
kha "lock screen"

# Status
kha status
```

**31 intents** supported — English and Azerbaijani Turkish. All resolved at Tier 0 with sub-millisecond latency.

## How it Works

```
User types: "open firefox"
    │
    ▼
Orchestrator ──► Rule Engine (Tier 0, 31 intents)
    │                │
    │           UNKNOWN? ──► Intent Classifier (Tier 1, LLM)
    │
    ▼
Registry ──► find capability "open_application" → desktop-agent
    │
    ▼
Policy Engine ──► allow/deny check (13 rules)
    │
    ▼
Agent ──► fork+exec firefox → "opened firefox"
```

## Daemon Services

| Daemon | Socket | Purpose |
|--------|--------|---------|
| `ai-orchestrator` | `/run/khazar/orchestrator.sock` | Intent routing + agent dispatch |
| `ai-rule-engine` | `/run/khazar/rule-engine.sock` | Tier 0 deterministic classifier |
| `ai-policy-engine` | `/run/khazar/policy-engine.sock` | Security policy enforcement |
| `ai-model-runtime` | `/run/khazar/model-runtime.sock` | LLM inference (GGUF/llama.cpp) |
| `ai-intent-classifier` | `/run/khazar/intent-classifier.sock` | Tier 1 LLM fallback |

## Agents

| Agent | Capability | Backend |
|-------|-----------|---------|
| `ai-desktop-agent` | open_application, close_application | fork+exec |
| `ai-package-agent` | install_package, remove_package, search, update | apt / pacman |
| `ai-network-agent` | network_management | nmcli |
| `ai-power-agent` | system_shutdown, reboot, suspend, lock | systemctl / loginctl |
| `ai-audio-agent` | volume_up, volume_down, mute | pactl |

## Repositories

| Repo | Purpose |
|------|---------|
| **[khazar-os](https://github.com/Khazar-System-Distribution/khazar-os)** | Distro — Fedora Silverblue downstream |
| [khazar-distro](https://github.com/Khazar-System-Distribution/khazar-distro) | AI Platform — C codebase, SDK, 11 components, 5 agents |

## Build from Source

```bash
# Clone the distro
git clone https://github.com/Khazar-System-Distribution/khazar-os
cd khazar-os

# Build container (Fedora host required)
podman build -t khazaros -f Containerfile .

# Build ISO
bootc-image-builder --type anaconda-iso localhost/khazaros:latest khazaros.iso
```

## Credits

- Built with [Universal Blue image template](https://github.com/ublue-os/image-template)
- Base: [quay.io/fedora/fedora-bootc:40](https://quay.io/repository/fedora/fedora-bootc)
- AI Platform: [Khazar System Distribution](https://github.com/Khazar-System-Distribution)

## License

GPL v3 — [LICENSE](LICENSE)
