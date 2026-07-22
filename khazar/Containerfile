# KhazarOS — Fedora Silverblue Distro Image
# Base: Fedora 40 Silverblue (official)
# Layer: Khazar AI platform + branding
#
# Build:  docker build -t khazaros -f Containerfile .
# ISO:    bootc-image-builder --type anaconda-iso docker-daemon:khazaros:latest khazaros.iso
#
# Based on: quay.io/fedora/fedora-bootc:40

FROM quay.io/fedora/fedora-bootc:40

# ── Khazar user ────────────────────────────
RUN groupadd -r khazar 2>/dev/null; \
    useradd -r -s /sbin/nologin -d /var/lib/khazar -g khazar khazar 2>/dev/null; \
    mkdir -p /var/lib/khazar/{bin,models} /run/khazar /etc/khazar/policies; \
    chown -R khazar:khazar /var/lib/khazar /run/khazar /etc/khazar; \
    true

# ── Khazar AI Platform (pre-compiled binaries) ──
COPY system/usr/local/bin/ /usr/local/bin/
RUN chmod +x /usr/local/bin/* 2>/dev/null; true

# ── Configs ───────────────────────────────
COPY system/etc/khazar/ /etc/khazar/
RUN chown -R khazar:khazar /etc/khazar

# ── CLI ───────────────────────────────────
RUN [ -f /usr/local/bin/kha ] && cp /usr/local/bin/kha /usr/bin/kha && chmod +x /usr/bin/kha; true

# ── systemd services ──────────────────────
COPY system/etc/systemd/system/ /etc/systemd/system/
RUN systemctl enable khazar.target 2>/dev/null; \
    systemctl enable ai-rule-engine.service 2>/dev/null; \
    systemctl enable ai-policy-engine.service 2>/dev/null; \
    systemctl enable ai-orchestrator.service 2>/dev/null; \
    systemctl enable ai-desktop-agent.service 2>/dev/null; \
    systemctl enable ai-package-agent.service 2>/dev/null; \
    systemctl enable ai-network-agent.service 2>/dev/null; \
    systemctl enable ai-power-agent.service 2>/dev/null; \
    systemctl enable ai-audio-agent.service 2>/dev/null; \
    true

# ── OS Identity ────────────────────────────
COPY system/etc/os-release /usr/lib/os-release 2>/dev/null; true
COPY system/etc/issue /etc/issue 2>/dev/null; true

# ── Branding ────────────────────────────────
COPY system/usr/share/ /usr/share/ 2>/dev/null; true

# ── GNOME Theme ────────────────────────────
COPY system/usr/share/themes/ /usr/share/themes/ 2>/dev/null; true
COPY system/usr/share/glib-2.0/schemas/ /usr/share/glib-2.0/schemas/ 2>/dev/null; true
RUN glib-compile-schemas /usr/share/glib-2.0/schemas/ 2>/dev/null; true

# ── Post-install setup script ──────────────
RUN echo '#!/bin/bash' > /usr/libexec/khazar-postinstall && \
    echo 'systemctl enable khazar.target 2>/dev/null' >> /usr/libexec/khazar-postinstall && \
    echo 'gsettings set org.gnome.desktop.interface gtk-theme "Khazar-dark" 2>/dev/null' >> /usr/libexec/khazar-postinstall && \
    echo 'gsettings set org.gnome.desktop.interface color-scheme "prefer-dark" 2>/dev/null' >> /usr/libexec/khazar-postinstall && \
    chmod +x /usr/libexec/khazar-postinstall

# ── Metadata ────────────────────────────────
LABEL distro.name="KhazarOS"
LABEL distro.version="0.1.0"
LABEL distro.family="Fedora Silverblue"
LABEL distro.homepage="https://github.com/Khazar-System-Distribution/khazar-distro"

CMD ["/sbin/init"]
