#!/bin/bash
# Interactive test — C əsaslı client ilə (UTF-8 təhlükəsiz)

RULE_SOCK="/tmp/ai-rule.sock"
RULE_BIN="../ai-rule-engine/ai-rule-engine"
RULE_TOOL="../ai-rule-engine/tools/interactive"
PID=""

cleanup() {
    [ -n "$PID" ] && kill "$PID" 2>/dev/null
    rm -f "$RULE_SOCK" /tmp/rule-interactive.toml
}
trap cleanup EXIT

# Build hər ikisini
make -C ../ai-rule-engine tools/interactive 2>/dev/null
make -C . -s 2>/dev/null

# Köhnə instance öldür
pkill -f "ai-rule-engine.*rule-interactive" 2>/dev/null || true
sleep 0.3
rm -f "$RULE_SOCK"

# Config yarat
cat > /tmp/rule-interactive.toml << EOF
socket_path="$RULE_SOCK"
max_connections=64
enable_cache=true
enable_regex=true
enable_tokens=true
enable_intent_table=true
enable_alias=true
enable_fuzzy=false
daemonize=false
log_level=4
EOF

# Rule engine başlat
"$RULE_BIN" /tmp/rule-interactive.toml > /dev/null 2>&1 &
PID=$!

# Socket hazır olana qədər gözlə
for i in $(seq 1 20); do
    [ -S "$RULE_SOCK" ] && break
    sleep 0.1
done

# C əsaslı interaktiv client
"$RULE_TOOL" "$RULE_SOCK"

# Təmizlik
kill "$PID" 2>/dev/null
rm -f "$RULE_SOCK" /tmp/rule-interactive.toml
