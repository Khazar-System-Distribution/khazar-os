# KhazarSystemDistro

AI-powered Linux Desktop — Fedora Silverblue + Local AI + GNOME.

Built with [Universal Blue](https://universal-blue.org/) image-template and [bootc](https://github.com/bootc-dev/bootc).

## Quick Start

```bash
sudo bootc switch ghcr.io/Khazar-System-Distribution/khazar-system:latest
sudo reboot
```

## Components

- **Khazar AI Platform** — Natural language desktop control (C11, offline)
- **Ollama** — Local LLM runtime
- **Fedora Silverblue** — Immutable, atomic updates
- **Bluefin** — GNOME desktop base
- **31 intents** — Application, package, network, power, audio control

## Usage

```bash
kha "open firefox"
kha "turn off wifi"
kha "volume up"
kha "update system"
kha status
```

## Build

```bash
# Local build
git clone https://github.com/Khazar-System-Distribution/khazar-system
cd khazar-system
just build
```

## License

Apache-2.0 — see [LICENSE](LICENSE)
