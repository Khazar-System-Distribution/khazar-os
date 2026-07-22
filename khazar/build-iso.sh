#!/bin/bash
# KhazarOS — REAL ISO Builder (Fedora 40+ host)
# Uses livecd-creator to build a Fedora-based Live ISO
#
# Requirements: Fedora 40+, root, 20GB disk
# Install: sudo dnf install livecd-tools spin-kickstarts lorax

set -e
V="${1:-0.1.0}"

echo "=============================================="
echo "  KhazarOS v${V} — Live ISO Builder"
echo "  Base: Fedora 40 + GNOME + Khazar AI"
echo "=============================================="
echo ""

# 1. Install tools
if ! command -v livecd-creator &>/dev/null; then
    echo "[*] Installing livecd-tools..."
    sudo dnf install -y livecd-tools spin-kickstarts lorax git
fi

# 2. Build Khazar
if [ ! -f components/orchestrator/ai-orchestrator ]; then
    echo "[1/3] Building Khazar platform..."
    make clean &>/dev/null || true
    make all
fi

# 3. Prepare overlay
echo "[2/3] Preparing overlay files..."
mkdir -p /tmp/khazar-bin /tmp/khazar-config /tmp/khazar-systemd

for comp in orchestrator rule-engine policy-engine model-runtime intent-classifier; do
    cp components/$comp/ai-$comp /tmp/khazar-bin/ 2>/dev/null || true
done
for agent in desktop package network power audio; do
    cp agents/${agent}-agent/ai-${agent}-agent /tmp/khazar-bin/ 2>/dev/null || true
done
cp distro/cli/kha /tmp/khazar-bin/ 2>/dev/null || true
cp distro/configs/*.toml /tmp/khazar-config/ 2>/dev/null || true
cp distro/systemd/*.service distro/systemd/khazar.target /tmp/khazar-systemd/ 2>/dev/null || true

chmod +x /tmp/khazar-bin/* 2>/dev/null || true
ls /tmp/khazar-bin/

# 4. Build ISO
echo "[3/3] Building ISO (this takes 5-10 minutes)..."
sudo livecd-creator \
    --config=os-build/khazaros.ks \
    --fslabel="KhazarOS" \
    --title="KhazarOS ${V}" \
    --product="KhazarOS" \
    --releasever=40 \
    --tmpdir=/tmp/livecd \
    --cache=/var/tmp/livecd-cache

# 5. Find ISO
ISO=$(find /tmp -name "*.iso" -type f 2>/dev/null | head -1)
if [ -n "$ISO" ]; then
    cp "$ISO" "khazaros-${V}-live-x86_64.iso"
    echo ""
    echo "=============================================="
    echo "  ISO READY: khazaros-${V}-live-x86_64.iso"
    echo "  Size: $(du -h khazaros-${V}-live-x86_64.iso | cut -f1)"
    echo "  SHA256: $(sha256sum khazaros-${V}-live-x86_64.iso | cut -d' ' -f1)"
    echo "=============================================="
    echo ""
    echo "  USB:  sudo dd if=khazaros-${V}-live-x86_64.iso of=/dev/sdX bs=4M"
    echo "  QEMU: qemu-system-x86_64 -m 4096 -enable-kvm -cdrom khazaros-${V}-live-x86_64.iso"
else
    echo "ERROR: ISO not found. Check /tmp/ for livecd build artifacts."
    exit 1
fi

# Cleanup
sudo rm -rf /tmp/khazar-* /tmp/livecd
