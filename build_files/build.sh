#!/bin/bash
set -ouex pipefail

# ── Copy system overlay files ──────────────────────
cp -avf "/ctx/system_files"/. /

# ── Create Khazar user ─────────────────────────────
groupadd -r khazar 2>/dev/null || true
useradd -r -s /sbin/nologin -d /var/lib/khazar -g khazar khazar 2>/dev/null || true
mkdir -p /var/lib/khazar/{bin,models} /run/khazar /etc/khazar/policies
chown -R khazar:khazar /var/lib/khazar /run/khazar /etc/khazar

# ── Install runtime dependencies ───────────────────
dnf5 install -y \
    pulseaudio-utils \
    NetworkManager-wifi \
    papirus-icon-theme \
    jetbrains-mono-fonts \
    dejavu-sans-fonts \
    python3 \
    zenity \
    2>/dev/null || dnf install -y \
    pulseaudio-utils \
    NetworkManager-wifi \
    papirus-icon-theme \
    jetbrains-mono-fonts \
    dejavu-sans-fonts \
    python3 \
    zenity

# ── Set permissions on Khazar binaries ─────────────
chmod +x /usr/local/bin/ai-* 2>/dev/null || true
chmod +x /usr/local/bin/kha 2>/dev/null || true
[ -f /usr/local/bin/kha ] && ln -sf /usr/local/bin/kha /usr/bin/kha

# ── Enable Khazar services ─────────────────────────
systemctl enable ai-rule-engine.service 2>/dev/null || true
systemctl enable ai-policy-engine.service 2>/dev/null || true
systemctl enable ai-orchestrator.service 2>/dev/null || true
systemctl enable ai-desktop-agent.service 2>/dev/null || true
systemctl enable ai-package-agent.service 2>/dev/null || true
systemctl enable ai-network-agent.service 2>/dev/null || true
systemctl enable ai-power-agent.service 2>/dev/null || true
systemctl enable ai-audio-agent.service 2>/dev/null || true
systemctl enable khazar.target 2>/dev/null || true

# ── OS Identity ────────────────────────────────────
cat > /usr/lib/os-release << 'KHIDENTITY'
NAME="KhazarOS"
VERSION="0.1.0 (Xezer)"
ID="khazaros"
ID_LIKE="fedora"
VERSION_ID="0.1.0"
PRETTY_NAME="KhazarOS 0.1.0"
ANSI_COLOR="0;31"
HOME_URL="https://github.com/Khazar-System-Distribution/khazar-os"
KHIDENTITY
echo "KhazarOS 0.1.0" > /etc/issue 2>/dev/null || true

# ── GNOME defaults ─────────────────────────────────
cat > /usr/share/glib-2.0/schemas/00-khazar.gschema.override << 'KHGSETTINGS'
[org.gnome.desktop.interface]
gtk-theme='Adwaita-dark'
color-scheme='prefer-dark'
font-name='DejaVu Sans 10'
KHGSETTINGS
glib-compile-schemas /usr/share/glib-2.0/schemas/ 2>/dev/null || true
