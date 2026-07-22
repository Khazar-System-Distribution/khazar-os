#!/bin/bash
# KhazarOS — First-time setup wizard
# Runs on first boot, installs Khazar AI system

set -e

echo "╔══════════════════════════════════════╗"
echo "║   KhazarOS First Boot Setup         ║"
echo "╚══════════════════════════════════════╝"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Root lazimdir. sudo ile tekrar calisdirin."
    exit 1
fi

# 1. Create khazar user
if ! id khazar &>/dev/null; then
    groupadd -r khazar
    useradd -r -s /sbin/nologin -d /var/lib/khazar -g khazar khazar
    echo "[+] khazar user created"
fi

# 2. Create directories
mkdir -p /run/khazar /var/lib/khazar/{models,bin} /etc/khazar/policies
chown -R khazar:khazar /run/khazar /var/lib/khazar /etc/khazar
echo "[+] directories created"

# 3. Install configs
if [ -d /usr/share/khazar/configs ]; then
    cp /usr/share/khazar/configs/*.toml /etc/khazar/ 2>/dev/null
    cp /usr/share/khazar/configs/*.policy.toml /etc/khazar/policies/ 2>/dev/null
    echo "[+] configs installed"
fi

# 4. Download AI model (if not present)
if [ ! -f /var/lib/khazar/models/default.gguf ]; then
    echo "[*] AI model not found."
    echo "    LLM modelini /var/lib/khazar/models/default.gguf yoluna kocurun."
    echo "    Meselen:"
    echo "    wget -O /var/lib/khazar/models/default.gguf <URL>"
fi

# 5. Enable services
echo "[*] Enabling Khazar daemons..."
systemctl daemon-reload
systemctl enable khazar.target --now

echo ""
echo "=== KhazarOS setup complete ==="
echo ""
echo "  CLI:      kha 'firefox ac'"
echo "  Status:   kha status"
echo "  Logs:     journalctl -u khazar.target -f"
echo ""
