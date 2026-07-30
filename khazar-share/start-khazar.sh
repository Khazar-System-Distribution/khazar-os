#!/bin/bash
# KhazarOS — Agent startup with registration

set +e
KH_SOCK="/run/khazar/orchestrator.sock"
ORCH_SOCK="/run/khazar/orchestrator.sock"

ln -sf "$KH_SOCK" "$ORCH_SOCK"

# Start daemons
ai-orchestrator /etc/khazar/orchestrator.toml &
sleep 2
ai-rule-engine /etc/khazar/rule-engine.toml &
ai-policy-engine /etc/khazar/policy-engine.toml &
ai-model-runtime /etc/khazar/model-runtime.toml &
ai-intent-classifier /etc/khazar/intent-classifier.toml &
sleep 2

# Start agents
ai-desktop-agent &
ai-package-agent &
ai-network-agent &
ai-power-agent &
ai-audio-agent &
sleep 2

for i in $(seq 1 10); do [ -S "$KH_SOCK" ] && break; sleep 1; done

register() {
    echo "$1" | socat - UNIX-CONNECT:"$KH_SOCK" > /dev/null 2>&1
    echo "  [+] $2 registered"
}
register '{"type":"register","name":"desktop-agent","version":"0.1.0","socket":"/tmp/ai-desktop-agent.sock","capabilities":["open_application","close_application"]}' "desktop-agent"
register '{"type":"register","name":"package-agent","version":"0.1.0","socket":"/run/ai-package-agent.sock","capabilities":["install_package","remove_package","search_package","system_update"]}' "package-agent"
register '{"type":"register","name":"network-agent","version":"0.1.0","socket":"/run/ai-network-agent.sock","capabilities":["network_management"]}' "network-agent"
register '{"type":"register","name":"power-agent","version":"0.1.0","socket":"/run/ai-power-agent.sock","capabilities":["system_management"]}' "power-agent"
register '{"type":"register","name":"audio-agent","version":"0.1.0","socket":"/run/ai-audio-agent.sock","capabilities":["audio_control"]}' "audio-agent"

echo "=== Khazar AI Platform Ready ==="
echo "Orchestrator: $KH_SOCK"
echo "Try: kha 'firefox ac' or kha status"

while true; do sleep 60; done
