#!/bin/bash
set -ouex pipefail

# ============================================================
# KhazarSystem — build.sh
# Faz 1..9 ardıcıllıqla icra olunur.
# ============================================================

# ---------- faz 1: system_files overlay ----------
cp -avf "/ctx/system_files"/. /

# ---------- faz 2: build alətləri (müvəqqəti) ----------
dnf5 install -y gcc make socat

# ---------- faz 3: Khazar platform build (lokal mənbədən) ----------
cp -r /ctx/khazar /tmp/khazar-src
make -C /tmp/khazar-src all
# ostree systems have /usr/local -> ../var/usrlocal symlink
install -d /var/usrlocal/bin /var/usrlocal/lib /var/usrlocal/include 2>/dev/null || mkdir -p /usr/local/bin /usr/local/lib /usr/local/include
make -C /tmp/khazar-src install DESTDIR=/
install -m 755 /tmp/khazar-src/distro/cli/kha /usr/local/bin/kha

# ---------- faz 4: Ollama (tam quraşdırma) ----------
curl -fsSL https://ollama.com/install.sh | sh
systemctl enable ollama.service

# ---------- faz 5: Khazar sistem istifadəçisi ----------
groupadd -r khazar 2>/dev/null || true
useradd -r -s /sbin/nologin -d /var/lib/khazar -g khazar khazar 2>/dev/null || true
mkdir -p /var/lib/khazar/{models,bin} /run/khazar /etc/khazar/policies
chown -R khazar:khazar /var/lib/khazar /run/khazar /etc/khazar

# ---------- faz 6: GNOME GSettings kompilyasiya ----------
if [ -f /usr/share/glib-2.0/schemas/00-khazar-defaults.gschema.override ]; then
    glib-compile-schemas /usr/share/glib-2.0/schemas/
fi

# ---------- faz 7: Plymouth boot animasiyası ----------
plymouth-set-default-theme khazar 2>/dev/null || true

# ---------- faz 8: systemd servislər ----------
systemctl enable khazar.target 2>/dev/null || true
systemctl enable khazar-identity.service 2>/dev/null || true
systemctl enable ai-rule-engine.service 2>/dev/null || true
systemctl enable ai-policy-engine.service 2>/dev/null || true
systemctl enable ai-orchestrator.service 2>/dev/null || true
systemctl enable ai-model-runtime.service 2>/dev/null || true
systemctl enable ai-intent-classifier.service 2>/dev/null || true
systemctl enable ai-desktop-agent.service 2>/dev/null || true
systemctl enable ai-package-agent.service 2>/dev/null || true
systemctl enable ai-network-agent.service 2>/dev/null || true
systemctl enable ai-power-agent.service 2>/dev/null || true
systemctl enable ai-audio-agent.service 2>/dev/null || true
systemctl enable podman.socket 2>/dev/null || true

# ---------- faz 9: təmizlik ----------
rm -rf /tmp/khazar-src
dnf5 remove -y gcc make 2>/dev/null || true
dnf5 clean all
umount /etc/systemd/resolve/stub-resolv.conf 2>/dev/null || true
rm -f /etc/systemd/resolve/stub-resolv.conf 2>/dev/null || true
ostree container commit 2>/dev/null || true
