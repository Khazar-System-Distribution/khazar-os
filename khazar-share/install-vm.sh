#!/bin/bash
# KhazarOS — VM Installer
# Run inside Fedora Live VM: sudo bash /mnt/khazar/install-vm.sh

set -e
SRC="/mnt/khazar"

if [ "$EUID" -ne 0 ]; then
    echo "Root ile isledin: sudo bash install-vm.sh"
    exit 1
fi

echo "=== KhazarOS VM Installation ==="
echo ""

# 1. Install required packages
echo "[1/5] Installing dependencies..."
dnf install -y gcc make socat 2>/dev/null || true

# 2. Create khazar user
echo "[2/5] Creating khazar user..."
if ! id khazar &>/dev/null; then
    groupadd -r khazar 2>/dev/null || true
    useradd -r -s /sbin/nologin -d /var/lib/khazar -g khazar khazar 2>/dev/null || true
fi
mkdir -p /var/lib/khazar/{models,bin} /run/khazar /etc/khazar/policies
chown -R khazar:khazar /var/lib/khazar /run/khazar /etc/khazar 2>/dev/null || true

# 3. Install binaries
echo "[3/5] Installing binaries..."
mkdir -p /usr/local/bin
cp "$SRC/bin/ai-orchestrator" "$SRC/bin/ai-rule-engine" "$SRC/bin/ai-policy-engine" \
   "$SRC/bin/ai-model-runtime" "$SRC/bin/ai-intent-classifier" \
   "$SRC/bin/ai-desktop-agent" "$SRC/bin/ai-package-agent" \
   "$SRC/bin/ai-network-agent" "$SRC/bin/ai-power-agent" "$SRC/bin/ai-audio-agent" \
   "$SRC/bin/kha" /usr/local/bin/
chmod +x /usr/local/bin/*
echo "  Binaries installed"

# 4. Install configs
echo "[4/5] Installing configs..."
cp "$SRC/config/"*.toml /etc/khazar/
cp "$SRC/config/policies/"* /etc/khazar/policies/ 2>/dev/null || true
echo "  Configs installed"

# 5. Install systemd units
echo "[5/5] Installing systemd units..."
cp "$SRC/systemd/"*.service "$SRC/systemd/khazar.target" /etc/systemd/system/
systemctl daemon-reload
systemctl enable khazar.target
echo "  systemd units installed"

echo ""
echo "=== KhazarOS installed! ==="
echo "  Start:  systemctl start khazar.target"
echo "  Test:   kha status"
echo "  Use:    kha 'firefox ac'"
