#!/bin/bash
# ============================================================
# AI Rule Engine + AI Orchestrator — Tam Integrasiya Testi
# ============================================================

ORCH_SOCK="/tmp/test-orch.sock"
RULE_SOCK="/tmp/test-rule.sock"
ORCH_BIN="./ai-orchestrator"
RULE_BIN="../ai-rule-engine/ai-rule-engine"
TMPDIR="/tmp/khazar-int-test"
PASS=0
FAIL=0
RULE_PID=""
ORCH_PID=""

cleanup() {
    echo ""
    echo "[temizlik]"
    [ -n "$ORCH_PID" ] && kill "$ORCH_PID" 2>/dev/null || true
    [ -n "$RULE_PID" ] && kill "$RULE_PID" 2>/dev/null || true
    [ -n "$ORCH_PID" ] && wait "$ORCH_PID" 2>/dev/null || true
    [ -n "$RULE_PID" ] && wait "$RULE_PID" 2>/dev/null || true
    rm -f "$ORCH_SOCK" "$RULE_SOCK"
    rm -rf "$TMPDIR"
}
trap cleanup EXIT INT TERM

mkdir -p "$TMPDIR"

# --- sock_send: Unix soketine JSON gonder, cavab al (retry ile) ---
sock_send() {
    local sock="$1" msg="$2"
    local resp
    for i in 1 2 3; do
        resp=$(echo "$msg" | nc -U -w 3 "$sock" 2>/dev/null)
        if [ -n "$resp" ] && [ "$resp" != "TIMEOUT" ]; then
            echo "$resp"
            return 0
        fi
        sleep 0.3
    done
    echo "TIMEOUT"
}

# --- Config fayllari ---
cat > "$TMPDIR/rule.toml" << EOF
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
log_file="$TMPDIR/rule.log"
cache_cleanup_interval=3600
EOF

cat > "$TMPDIR/orch.toml" << EOF
socket_path="$ORCH_SOCK"
max_connections=64
request_timeout_ms=5000
enable_metrics=true
log_level=4
rule_engine_socket="$RULE_SOCK"
EOF

echo "============================================================"
echo "   AI Rule Engine + AI Orchestrator Integrasiya Testi"
echo "============================================================"
echo ""

# --- Build ---
echo "[build] Kompilyasiya..."
make -C ../ai-rule-engine clean > /dev/null 2>&1
make -C ../ai-rule-engine > /dev/null 2>&1
make clean > /dev/null 2>&1
make > /dev/null 2>&1
echo "[build] Her ikisi hazirdir"
echo ""

# --- Start Rule Engine ---
echo "[start] AI Rule Engine basladilir..."
unlink "$RULE_SOCK" 2>/dev/null || true
"$RULE_BIN" "$TMPDIR/rule.toml" > /dev/null 2>&1 &
RULE_PID=$!
sleep 1

if ! kill -0 $RULE_PID 2>/dev/null; then
    echo "FAIL: Rule Engine baslamadi!"
    cat "$TMPDIR/rule.log" 2>/dev/null
    exit 1
fi
echo "[start] Rule Engine PID=$RULE_PID"

# --- Start Orchestrator ---
echo "[start] AI Orchestrator basladilir..."
unlink "$ORCH_SOCK" 2>/dev/null || true
"$ORCH_BIN" "$TMPDIR/orch.toml" > /dev/null 2>&1 &
ORCH_PID=$!
sleep 1

if ! kill -0 $ORCH_PID 2>/dev/null; then
    echo "FAIL: Orchestrator baslamadi!"
    exit 1
fi
echo "[start] Orchestrator PID=$ORCH_PID"
echo ""

# ==== TEST FUNKSIYASI ====
run_test() {
    local name="$1" query="$2" pattern="$3" sock="$4"
    printf "  TEST: %-38s " "${name:0:38}"
    local resp
    resp=$(sock_send "$sock" "{\"id\":1,\"type\":\"request\",\"query\":\"$query\"}" 2>/dev/null)
    if echo "$resp" | grep -q "$pattern"; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL"
        echo "        query   : $query"
        echo "        expected: $pattern"
        echo "        response: ${resp:0:120}"
        FAIL=$((FAIL + 1))
    fi
    sleep 0.15
}

ping_test() {
    local name="$1" sock="$2"
    printf "  TEST: %-38s " "${name:0:38}"
    local resp
    resp=$(sock_send "$sock" '{"type":"ping"}' 2>/dev/null)
    if echo "$resp" | grep -q "alive"; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL (resp: ${resp:0:80})"
        FAIL=$((FAIL + 1))
    fi
}

# ============================================================
echo "=== 1. Birbasa Rule Engine Testleri ==="
echo ""

ping_test "rule engine ping" "$RULE_SOCK"
run_test "rule: firefox aç"       "firefox aç"       "open_application" "$RULE_SOCK"
run_test "rule: telegram aç"      "telegram aç"      "open_application" "$RULE_SOCK"
run_test "rule: wifi söndür"      "wifi söndür"      "network_disable"  "$RULE_SOCK"
run_test "rule: wifi aç"          "wifi aç"          "network_enable"   "$RULE_SOCK"
run_test "rule: sistemi yenilə"   "sistemi yenilə"   "system_update"    "$RULE_SOCK"
run_test "rule: steam quraşdır"   "steam quraşdır"   "install_package"  "$RULE_SOCK"
run_test "rule: shutdown"         "shutdown"         "system_shutdown"  "$RULE_SOCK"
run_test "rule: reboot"           "reboot"           "system_reboot"    "$RULE_SOCK"
run_test "rule: screenshot"       "screenshot"       "screenshot"       "$RULE_SOCK"
run_test "rule: lock"             "lock"             "system_lock"      "$RULE_SOCK"
run_test "rule: unknown query"    "xyzabc_unknown"   "unknown"          "$RULE_SOCK"

echo ""
echo "=== 2. Orchestrator → Rule Engine Testleri ==="
echo "     (orchestrator rule_client modulu ile sorgu gonderir)"
echo ""

ping_test "orchestrator ping" "$ORCH_SOCK"
run_test "orch: firefox aç"       "firefox aç"       "open_application" "$ORCH_SOCK"
run_test "orch: telegram aç"      "telegram aç"      "open_application" "$ORCH_SOCK"
run_test "orch: wifi söndür"      "wifi söndür"      "network_disable"  "$ORCH_SOCK"
run_test "orch: wifi aç"          "wifi aç"          "network_enable"   "$ORCH_SOCK"
run_test "orch: sistemi yenilə"   "sistemi yenilə"   "system_update"    "$ORCH_SOCK"
run_test "orch: steam quraşdır"   "steam quraşdır"   "install_package"  "$ORCH_SOCK"
run_test "orch: chrome aç"        "chrome aç"        "open_application" "$ORCH_SOCK"
run_test "orch: spotify aç"       "spotify aç"       "open_application" "$ORCH_SOCK"
run_test "orch: gimp aç"          "gimp aç"          "open_application" "$ORCH_SOCK"
run_test "orch: vlc aç"           "vlc aç"           "open_application" "$ORCH_SOCK"
run_test "orch: ekranı kilidlə"   "ekranı kilidlə"   "system_lock"      "$ORCH_SOCK"
run_test "orch: unknown query"    "xyz_unknown_abc"  "no intent matched" "$ORCH_SOCK"

echo ""
echo "=== 3. End-to-end: Rule Engine metrics via orchestrator ==="
echo ""

echo "  Bir nece sorgu gonderilir..."
for q in "firefox aç" "wifi söndür" "steam quraşdır" "sistemi yenilə" "telegram aç"; do
    sock_send "$ORCH_SOCK" "{\"id\":1,\"type\":\"request\",\"query\":\"$q\"}" > /dev/null 2>&1
done

printf "  TEST: rule engine metrics %-18s " ""
resp=$(sock_send "$RULE_SOCK" '{"type":"metrics"}' 2>/dev/null)
if echo "$resp" | grep -q "total_requests" && echo "$resp" | grep -q "cache_hits"; then
    echo "PASS"
    PASS=$((PASS + 1))
else
    echo "FAIL"
    FAIL=$((FAIL + 1))
fi

printf "  TEST: rule engine version %-18s " ""
resp=$(sock_send "$RULE_SOCK" '{"type":"version"}' 2>/dev/null)
if echo "$resp" | grep -q '"version"'; then
    echo "PASS"
    PASS=$((PASS + 1))
else
    echo "FAIL"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "============================================================"
echo "FULL INTEGRASIYA NETICE: $PASS PASS, $FAIL FAIL"
echo "============================================================"
echo ""

if [ $FAIL -gt 0 ]; then
    echo "--- Son 10 Rule Engine log setri ---"
    tail -10 "$TMPDIR/rule.log" 2>/dev/null || true
fi

exit $FAIL
