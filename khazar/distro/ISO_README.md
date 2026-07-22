# KhazarOS — AI-powered Linux Desktop

Fedora Silverblue əsasında qurulmuş **AI əməliyyat sistemi**.

Bütün sistem idarəsi səsli və ya mətn əmrləri ilə:

```
$ kha "firefox aç"
$ kha "wifi söndür"
$ kha "səs artır"
$ kha "sistemi yenilə"
```

## Arxitektura

```
┌──────────────────────────────────────────┐
│           KhazarOS Desktop               │
│                                          │
│  ┌─────────┐  ┌──────────────────────┐   │
│  │  GNOME  │  │   Khazar Daemon      │   │
│  │  Shell  │  │   (systemd services)  │   │
│  │         │  │                      │   │
│  │ Panel   │  │  orchestrator  ──────│───│── rule-engine
│  │ Icon ◄─┼──┤  policy-engine ──────│───│── model-runtime
│  │         │  │  intent-classifier   │   │
│  │ Ctrl+   │  │                      │   │
│  │ Space ◄─┼──┤  5 Agents:           │   │
│  │         │  │  desktop package     │   │
│  │         │  │  network power audio │   │
│  └─────────┘  └──────────────────────┘   │
│                                          │
│  Əsas: Fedora Silverblue (immutable)     │
│  Kernel: Linux $(uname -r)               │
└──────────────────────────────────────────┘
```

## Build & Quraşdırma

### ISO qurmaq

```bash
cd khazar
make -C distro build-iso
# → distro/iso/output/khazaros.iso
```

### Bootable USB

```bash
make -C distro flash DISK=/dev/sdX
```

### QEMU-da test

```bash
make -C distro test-qemu
```

### Mövcud Fedora-ya quraşdırma

```bash
# RPM ilə
sudo rpm-ostree install khazar

# Və ya manual
sudo bash distro/installer/setup.sh
```

## İstifadə

```bash
kha "firefox aç"       # Tətbiq aç
kha "wifi söndür"      # WiFi bağla
kha "səs artır"        # Səs +5%
kha "ekran kilidlə"    # Sistemi kilidlə
kha "shutdown"         # Söndür (policy yoxlayır)
kha status             # Sistem statusu
```

## Sistem Tələbləri

- 64-bit CPU (x86_64)
- 4 GB RAM (tövsiyə: 8 GB)
- 20 GB disk
- GGUF AI modeli (LLM üçün, isteğe bağlı)

## Lisensiya

GPL v3
