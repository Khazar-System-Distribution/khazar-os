# Changelog

## [0.1.0] — 2025-07-21

### Added
- **SDK**: IPC client/server, protocol, logger, agent lifecycle, events (22 test)
- **Orchestrator**: Tier 0+1 intent routing, registry, policy client, agent dispatch (v0.3)
- **Rule Engine**: Deterministic intent classifier — 31 intent, 5 pipeline stages (49 test)
- **Policy Engine**: Security policy enforcement — 13 rules, fnmatch matching (7 test)
- **Model Runtime**: LLM inference server — mock backend + llama.cpp stub (4 test)
- **Intent Classifier**: Tier 1 LLM fallback with 32 keyword rules (12 test)
- **Desktop Agent**: Application open/close via fork+exec
- **Package Agent**: Install/remove/search/update via apt/pacman
- **Network Agent**: WiFi/ethernet control via nmcli
- **Power Agent**: Shutdown/reboot/suspend/lock via systemctl/loginctl
- **Audio Agent**: Volume/mute control via pactl
- **IPC Protocol**: Full documentation — 11 message types, JSON Schemas, RPC
- **GNOME Integration**: Khazar Dark theme (Shell + GTK4), panel extension, GSettings
- **Distro Build**: bootc Containerfile, ISO builder, systemd services, RPM spec
- **CLI**: kha command-line tool
- **Branding**: GRUB theme, Plymouth boot animation, GDM login screen, wallpapers
- **CI**: GitHub Actions build workflow
