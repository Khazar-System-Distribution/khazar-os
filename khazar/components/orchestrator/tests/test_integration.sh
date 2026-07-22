#!/bin/sh
set -e

SOCKET="/tmp/ai-orchestrator-test.sock"
CONFIG="/tmp/ai-orchestrator-test.toml"
PASS=0
FAIL=0

cleanup() {
    kill $ORCH_PID 2>/dev/null || true
    kill $AGENT_PID 2>/dev/null || true
    rm -f "$SOCKET" "$CONFIG"
}

trap cleanup EXIT INT TERM

cat > "$CONFIG" << EOF
socket_path = "$SOCKET"
max_connections = 256
request_timeout_ms = 3000
enable_metrics = true
log_level = "ERROR"
EOF

echo "=== Integration Tests ==="

echo "1. Starting orchestrator..."
../ai-orchestrator "$CONFIG" &
ORCH_PID=$!
sleep 1

echo "2. Testing ping..."
RESULT=$(echo '{"type":"ping"}' | python3 -c "
import socket
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.settimeout(3)
s.connect('$SOCKET')
s.send(b'{\"type\":\"ping\"}')
print(s.recv(1024).decode())
s.close()
")
if echo "$RESULT" | grep -q "alive"; then
    echo "   PING: OK ($RESULT)"
    PASS=$((PASS+1))
else
    echo "   PING: FAIL ($RESULT)"
    FAIL=$((FAIL+1))
fi

echo "3. Testing intent routing..."
RESULT=$(echo '{"type":"request","query":"firefox ac"}' | python3 -c "
import socket
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.settimeout(3)
s.connect('$SOCKET')
s.send(b'{\"type\":\"request\",\"query\":\"firefox ac\"}')
print(s.recv(4096).decode())
s.close()
")
if echo "$RESULT" | grep -q -E "routed to|no agent available"; then
    echo "   ROUTE: OK ($RESULT)"
    PASS=$((PASS+1))
else
    echo "   ROUTE: FAIL ($RESULT)"
    FAIL=$((FAIL+1))
fi

echo "4. Testing unknown query..."
RESULT=$(echo '{"type":"request","query":"selam"}' | python3 -c "
import socket
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.settimeout(3)
s.connect('$SOCKET')
s.send(b'{\"type\":\"request\",\"query\":\"selam\"}')
print(s.recv(4096).decode())
s.close()
")
if echo "$RESULT" | grep -q "query received"; then
    echo "   UNKNOWN: OK ($RESULT)"
    PASS=$((PASS+1))
else
    echo "   UNKNOWN: FAIL ($RESULT)"
    FAIL=$((FAIL+1))
fi

kill $ORCH_PID 2>/dev/null
wait $ORCH_PID 2>/dev/null

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
exit $FAIL
