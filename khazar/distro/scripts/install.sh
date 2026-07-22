#!/bin/bash
# Khazar System Distribution - Install Script
# Fedora Silverblue / Atomic Desktop install

set -e

PREFIX="${PREFIX:-/usr/local}"
KH_CONF="${KH_CONF:-/etc/khazar}"
KH_RUN="${KH_RUN:-/run/khazar}"
KH_USER="khazar"
KH_GROUP="khazar"

echo "=== Khazar System Distribution v0.1 ==="
echo ""

# 1. Create user
if ! id "$KH_USER" &>/dev/null; then
    useradd -r -s /sbin/nologin -d /var/lib/khazar "$KH_USER"
    echo "[+] Created user: $KH_USER"
fi

# 2. Build from source
echo "[*] Building from source..."
cd "$(dirname "$0")/.."
make clean && make all

# 3. Install binaries
echo "[*] Installing binaries to $PREFIX/bin..."
mkdir -p "$PREFIX/bin"
for bin in ai-orchestrator ai-rule-engine ai-policy-engine ai-model-runtime ai-intent-classifier; do
    if [ -f "components/${bin#ai-}/$bin" ]; then
        install -m 755 "components/${bin#ai-}/$bin" "$PREFIX/bin/"
        echo "  [+] $bin"
    fi
done
for agent in desktop package network power audio; do
    bin="ai-${agent}-agent"
    if [ -f "agents/${agent}-agent/$bin" ]; then
        install -m 755 "agents/${agent}-agent/$bin" "$PREFIX/bin/"
        echo "  [+] $bin"
    fi
done

# 4. Install libraries
mkdir -p "$PREFIX/lib" "$PREFIX/include/ai-sdk"
install -m 644 sdk/libai-sdk.a "$PREFIX/lib/"
cp sdk/include/*.h "$PREFIX/include/ai-sdk/"
echo "[+] SDK installed"

# 5. Install configs
mkdir -p "$KH_CONF"
for comp in orchestrator rule-engine policy-engine model-runtime intent-classifier; do
    if [ -f "components/${comp}/config.toml.example" ]; then
        cp "components/${comp}/config.toml.example" "$KH_CONF/${comp}.toml"
        echo "  [+] $KH_CONF/${comp}.toml"
    fi
done

# 6. Install systemd units
if [ -d /etc/systemd/system ]; then
    echo "[*] Installing systemd units..."
    cp distro/systemd/*.service distro/systemd/khazar.target /etc/systemd/system/
    systemctl daemon-reload
    echo "[+] systemd units installed"

    echo ""
    echo "=== Enable at boot: ==="
    echo "  systemctl enable khazar.target --now"
fi

# 7. Done
echo ""
echo "=== Installation complete ==="
echo "Run:  systemctl enable khazar.target --now"
echo "CLI:  kha 'firefox ac'"
