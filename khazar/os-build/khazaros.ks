# KhazarOS — Fedora 40 Live ISO
# Built with: livecd-creator -c khazaros.ks

lang en_US.UTF-8
keyboard us
timezone UTC
selinux --enforcing
firewall --enabled
bootloader --timeout=5
services --enabled=NetworkManager,sshd

# Partitioning
part / --size 8192 --fstype ext4

repo --name=fedora --baseurl=https://mirrors.kernel.org/fedora/releases/40/Everything/x86_64/os/
repo --name=fedora-updates --baseurl=https://mirrors.kernel.org/fedora/updates/40/Everything/x86_64/

%packages
@core
@standard
@workstation-product-environment
@gnome-desktop
@networkmanager-submodules
@multimedia

-fedora-release-identity
-fedora-logos
-fedora-workstation-backgrounds

gcc
make
git
pulseaudio-utils
NetworkManager-wifi
dejavu-sans-fonts
jetbrains-mono-fonts
papirus-icon-theme
socat
python3
zenity

%end

%post --erroronfail
#!/bin/bash

# ── Khazar user ────────────────────
groupadd -r khazar 2>/dev/null || true
useradd -r -s /sbin/nologin -d /var/lib/khazar -g khazar khazar 2>/dev/null || true
mkdir -p /var/lib/khazar/{bin,models} /run/khazar /etc/khazar/policies
chown -R khazar:khazar /var/lib/khazar /run/khazar /etc/khazar

# ── OS Identity ─────────────────────
cat > /etc/os-release << 'KHIDENTITY'
NAME="KhazarOS"
VERSION="0.1.0 (Xezer)"
ID="khazaros"
ID_LIKE="fedora"
VERSION_ID="0.1.0"
PRETTY_NAME="KhazarOS 0.1.0"
ANSI_COLOR="0;31"
HOME_URL="https://github.com/Khazar-System-Distribution/khazar-distro"
KHIDENTITY

echo "KhazarOS 0.1.0" > /etc/issue

# ── Install Khazar binaries ─────────
if [ -d /tmp/khazar-bin ] && [ "$(ls -A /tmp/khazar-bin 2>/dev/null)" ]; then
    cp /tmp/khazar-bin/* /usr/local/bin/ 2>/dev/null || true
    chmod +x /usr/local/bin/ai-* 2>/dev/null || true
    [ -f /tmp/khazar-bin/kha ] && cp /tmp/khazar-bin/kha /usr/bin/ && chmod +x /usr/bin/kha
fi

# ── Configs ─────────────────────────
if [ -d /tmp/khazar-config ]; then
    cp /tmp/khazar-config/*.toml /etc/khazar/ 2>/dev/null || true
fi

# ── systemd services ────────────────
if [ -d /tmp/khazar-systemd ]; then
    cp /tmp/khazar-systemd/*.service /etc/systemd/system/ 2>/dev/null || true
    cp /tmp/khazar-systemd/khazar.target /etc/systemd/system/ 2>/dev/null || true
    systemctl enable khazar.target 2>/dev/null || true
fi

# ── GNOME defaults ──────────────────
cat > /usr/share/glib-2.0/schemas/00-khazar.gschema.override << 'KHGSETTINGS'
[org.gnome.desktop.interface]
gtk-theme='Adwaita-dark'
color-scheme='prefer-dark'
font-name='DejaVu Sans 10'

[org.gnome.shell]
favorite-apps=['org.mozilla.firefox.desktop', 'org.gnome.Nautilus.desktop', 'org.gnome.Console.desktop', 'org.gnome.Software.desktop', 'org.gnome.Settings.desktop']
KHGSETTINGS
glib-compile-schemas /usr/share/glib-2.0/schemas/ 2>/dev/null || true

# ── Cleanup ─────────────────────────
rm -rf /tmp/khazar-*
%end
