#!/bin/bash
# Install KhazarOS — Full Installer Script
# Usage: sudo bash install.sh

set -e
KH_SRC="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="/usr/local"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║                                                          ║"
echo "║               KhazarOS — Quraşdırıcı                     ║"
echo "║               AI Desktop OS v0.1.0                       ║"
echo "║                                                          ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

if [ "$EUID" -ne 0 ]; then
    echo "❌ Root ilə işlədin: sudo bash install.sh"
    exit 1
fi

# 1. System identity
echo "[1/10] Sistem identikliyi..."
mkdir -p /usr/share/khazar/configs
cp "$KH_SRC/distro/configs/os-release" /etc/os-release
cp "$KH_SRC/distro/configs/issue" /etc/issue
ln -sf khazar-release /etc/system-release 2>/dev/null || true

# 2. Create khazar user
echo "[2/10] Khazar istifadəçisi..."
if ! id khazar &>/dev/null; then
    groupadd -r khazar
    useradd -r -s /sbin/nologin -d /var/lib/khazar -g khazar khazar
fi
mkdir -p /run/khazar /var/lib/khazar/{models,bin} /etc/khazar/policies
chown -R khazar:khazar /run/khazar /var/lib/khazar /etc/khazar

# 3. Build & install binaries
echo "[3/10] Qurulur (make all)..."
cd "$KH_SRC"
make clean &>/dev/null
make all
mkdir -p "$PREFIX/bin" "$PREFIX/lib"
for comp in orchestrator rule-engine policy-engine model-runtime intent-classifier; do
    bin="ai-$comp"
    if [ -f "components/$comp/$bin" ]; then
        install -m 755 "components/$comp/$bin" "$PREFIX/bin/"
    fi
done
for agent in desktop package network power audio; do
    bin="ai-${agent}-agent"
    if [ -f "agents/${agent}-agent/$bin" ]; then
        install -m 755 "agents/${agent}-agent/$bin" "$PREFIX/bin/"
    fi
done
install -m 644 sdk/libai-sdk.a "$PREFIX/lib/"
echo "  [+] Binaries installed"

# 4. Configs
echo "[4/10] Konfiqurasiya faylları..."
mkdir -p /etc/khazar/policies
cp "$KH_SRC/distro/configs/"*.toml /etc/khazar/ 2>/dev/null || true
cp "$KH_SRC/distro/configs/default.policy.toml" /etc/khazar/policies/ 2>/dev/null || true

# 5. systemd
echo "[5/10] systemd servisləri..."
cp "$KH_SRC/distro/systemd/"*.service /etc/systemd/system/
cp "$KH_SRC/distro/systemd/khazar.target" /etc/systemd/system/
systemctl daemon-reload

# 6. GRUB
echo "[6/10] GRUB tema..."
if [ -d /boot/grub2 ]; then
    mkdir -p /boot/grub2/themes/khazar
    cp "$KH_SRC/distro/branding/grub/"* /boot/grub2/themes/khazar/ 2>/dev/null || true
    grep -q "GRUB_THEME" /etc/default/grub || echo 'GRUB_THEME="/boot/grub2/themes/khazar/theme.txt"' >> /etc/default/grub
fi

# 7. Plymouth
echo "[7/10] Plymouth boot animasiyası..."
mkdir -p /usr/share/plymouth/themes/khazar
cp "$KH_SRC/distro/branding/plymouth/"* /usr/share/plymouth/themes/khazar/ 2>/dev/null || true
plymouth-set-default-theme khazar 2>/dev/null || true

# 8. GDM
echo "[8/10] GDM login ekranı..."
cp "$KH_SRC/distro/branding/gdm/khazar-gdm.css" /usr/share/gnome-shell/theme/ 2>/dev/null || true
mkdir -p /usr/share/backgrounds/khazar

# 9. GNOME
echo "[9/10] GNOME özəlləşdirmə..."
bash "$KH_SRC/distro/gnome/install.sh" 2>/dev/null || true

# 10. CLI
echo "[10/10] CLI aləti..."
cp "$KH_SRC/distro/cli/kha" "$PREFIX/bin/"
chmod +x "$PREFIX/bin/kha"

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║          KhazarOS Quraşdırıldı!                         ║"
echo "║                                                          ║"
echo "║  Aktivləşdir:  systemctl enable khazar.target --now      ║"
echo "║  İstifadə:     kha 'firefox aç'                         ║"
echo "║  Status:       kha status                               ║"
echo "╚══════════════════════════════════════════════════════════╝"
