# KhazarSystemDistro — Fedora Silverblue + Khazar AI

Linux əməliyyat sistemi. Fedora Silverblue əsaslı, Khazar AI inteqrasiyalı.

## Xüsusiyyətlər

```
"firefox aç"           → Firefox açılır
"wifi söndür"          → WiFi bağlanır
"səs artır"            → Səs +5%
"sistemi yenilə"       → apt/pacman update
"ekran kilidlə"        → Sistem kilidlənir
"shutdown"             → (policy: DENY — root tələb olunur)
```

## Sürətli Başlanğıc

```bash
# Reponu klonla
git clone https://github.com/Khazar-System-Distribution/khazar
cd khazar

# Qur
make all

# Test
make -C sdk test

# Quraşdır
sudo bash distro/scripts/install.sh
sudo systemctl enable khazar.target --now

# İstifadə
kha "firefox aç"
kha status
```

## Struktur

```
khazar/
├── sdk/                    ← Agent SDK
├── components/             ← Server komponentlər
├── agents/                 ← Agentlər
├── protocol/               ← IPC sənədlər
├── distro/
│   ├── systemd/            ← systemd servis faylları
│   ├── cli/kha             ← CLI alət
│   ├── scripts/install.sh  ← Quraşdırıcı
│   └── rpm/khazar.spec     ← RPM spesifikasiyası
└── docs/                   ← Əlavə sənədlər
```

## Tələblər

- Fedora 39+ (Silverblue və ya Workstation)
- gcc, make, systemd
- socat, python3
- nmcli (şəbəkə agenti üçün)
- pactl (səs agenti üçün)
- apt və ya pacman (paket agenti üçün)

## Lisensiya

GPL v3
