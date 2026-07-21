# KhazarOS

**AI-powered Fedora Silverblue downstream.** Built with [Universal Blue](https://universal-blue.org/) image template.

## What is this?

KhazarOS takes Fedora Silverblue 40 and adds:

| Layer | What |
|-------|------|
| **Khazar AI** | 11 daemon services (systemd), natural language system control |
| **CLI** | `kha "open firefox"` — type commands in English or Turkish |
| **5 Agents** | desktop, package, network, power, audio |
| **Policy Engine** | security-first command execution (allow/deny) |
| **GNOME** | dark theme, panel icon extension |

## Install

### Rebase from Fedora Silverblue

```bash
sudo rpm-ostree rebase ostree-unverified-registry:ghcr.io/khazar-system-distribution/khazaros:latest
systemctl reboot
```

### ISO

Download from [Releases](https://github.com/Khazar-System-Distribution/khazar-os/releases).

```bash
sudo dd if=khazaros-latest-x86_64.iso of=/dev/sdX bs=4M status=progress
```

## Usage

```bash
kha "open firefox"
kha "turn off wifi"  
kha "volume up"
kha "update system"
kha shutdown          # denied by policy
kha status
```

## Architecture

```
FROM quay.io/fedora/fedora-bootc:40
    │
    ├── build.sh → installs Khazar binaries + systemd services
    ├── system_files/usr/local/bin/ → 11 Khazar daemons
    ├── system_files/etc/khazar/ → configs + policy rules
    ├── system_files/etc/systemd/system/ → service units
    └── OS identity → /etc/os-release: "KhazarOS"
    │
    = ostree container image + bootable ISO
```

## How it's built

- **Template**: [ublue-os/image-template](https://github.com/ublue-os/image-template) (same as Bazzite, Bluefin, Aurora)
- **Base**: `quay.io/fedora/fedora-bootc:40` (Fedora Silverblue)
- **CI**: GitHub Actions → podman build → ghcr.io → bootc-image-builder → ISO
- **AI Platform**: [khazar-distro](https://github.com/Khazar-System-Distribution/khazar-distro)

## Repositories

| Repo | Purpose |
|------|---------|
| [khazar-distro](https://github.com/Khazar-System-Distribution/khazar-distro) | AI platform (C codebase, 11 components, 5 agents) |
| **khazar-os** (this repo) | Fedora Silverblue downstream distro |

## License

GPL v3
