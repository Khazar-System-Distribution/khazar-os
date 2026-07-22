# Khazar — Fedora Silverblue Inteqrasiyası

## Arxitektura

```
┌─────────────────────────────────────────────────┐
│              Fedora Silverblue                   │
│  ┌───────────────────────────────────────────┐  │
│  │         rpm-ostree (immutable base)       │  │
│  │  ┌─────────────────────────────────────┐  │  │
│  │  │     Khazar Layer (rpm-ostree)       │  │  │
│  │  │  ┌───────────────────────────────┐  │  │  │
│  │  │  │    Khazar AI Platform         │  │  │  │
│  │  │  │    • systemd services         │  │  │  │
│  │  │  │    • CLI tool (kha)           │  │  │  │
│  │  │  │    • AI agents                │  │  │  │
│  │  │  └───────────────────────────────┘  │  │  │
│  │  └─────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────┘  │
│                                                 │
│  İstifadəçi:  $ kha "firefox aç"               │
│               $ kha "wifi söndür"               │
│               $ kha "səs artır"                 │
└─────────────────────────────────────────────────┘
```

## Quraşdırma (3 yol)

### Yol 1: rpm-ostree Layer (tövsiyə)

```bash
# RPM paketini qur
fedpkg --release f39 local

# rpm-ostree-yə əlavə et
rpm-ostree install ./khazar-0.1.0-1.fc39.x86_64.rpm

# Yenidən başlat
systemctl reboot

# Aktivləşdir
systemctl enable khazar.target --now
```

### Yol 2: Container/Flatpak + Socket Mount

```bash
# Khazar container-ini qur
podman build -t khazar .

# Socket üçün volume yarat
mkdir -p ~/.local/share/khazar/run

# Container-i işə sal
podman run -d --name khazar \
  -v ~/.local/share/khazar/run:/run/khazar \
  -v /run/user/1000/bus:/run/user/1000/bus \
  khazar

# CLI üçün alias
alias kha='podman exec -i khazar kha'
```

### Yol 3: Əl ilə Quraşdırma

```bash
git clone https://github.com/Khazar-System-Distribution/khazar
cd khazar
make all
sudo make install
sudo systemctl enable khazar.target --now
```

## Silverblue üçün Xüsusi Konfiqurasiya

Silverblue-da `/usr` oxunaqlıdır (read-only). Buna görə:

| Fayl | Silverblue yolu | Ənənəvi yolu |
|------|----------------|--------------|
| Binary | `/var/lib/khazar/bin/` | `/usr/local/bin/` |
| Config | `/etc/khazar/` | `/etc/khazar/` |
| Socket | `/run/khazar/` | `/run/khazar/` |
| Model | `/var/lib/khazar/models/` | `/usr/share/ai-models/` |

`/var` yazılaqlıdır — binary və modellər üçün əlverişlidir.

## İSO Qurma

Khazar inteqrasiya olunmuş xüsusi Fedora Silverblue ISO-su:

```bash
# 1. Fedora-nın osbuild-composer ilə blueprint yarat
cat > khazar-blueprint.toml << 'EOF'
name = "khazar-silverblue"
description = "Fedora Silverblue with Khazar AI"
version = "0.1.0"

[[packages]]
name = "khazar"
version = "*"

[[customizations.user]]
name = "user"
password = "$6$..."

[[customizations.services]]
enabled = ["khazar.target"]
EOF

# 2. İSO qur
composer-cli blueprints push khazar-blueprint.toml
composer-cli compose start khazar-silverblue image-installer
composer-cli compose image <ID> --filename khazar.iso
```

Və ya sadə yol — `bootc` ilə:

```bash
# Containerfile yarat
cat > Containerfile << 'EOF'
FROM quay.io/fedora/fedora-bootc:39
RUN rpm-ostree install khazar
RUN systemctl enable khazar.target
EOF

# Bootable container qur
podman build -t khazar-bootc .
bootc switch --transport oci khazar-bootc
```

## CLI İstifadə

```bash
# Sadə əmrlər
kha "firefox aç"
kha "wifi söndür"
kha "səs artır"
kha "ekran kilidlə"

# Status
kha status

# Sistemi yenidən başlat
kha restart
```

## GNOME İnteqrasiyası (gələcək)

```
┌─────────────────────────┐
│   GNOME Shell           │
│  ┌───────────────────┐  │
│  │ Khazar Extension  │  │  ← Quick Settings toggle
│  │  • Microfon icon  │  │  ← Voice input
│  │  • Command bar    │  │  ← Ctrl+Space shortcut
│  └───────────────────┘  │
└─────────────────────────┘
```
