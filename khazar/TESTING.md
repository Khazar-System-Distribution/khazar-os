# Testing KhazarOS

Test KhazarOS before installing on hardware. Three options from fastest to most realistic.

## Option A: QEMU/KVM (Recommended)

Fastest with near-native performance via KVM acceleration.

```bash
# Install QEMU
sudo dnf install qemu-kvm qemu-system-x86_64

# Download the ISO from GitHub Releases
wget https://github.com/Khazar-System-Distribution/khazar/releases/latest/download/khazaros.iso

# Run with 4GB RAM and KVM
qemu-system-x86_64 \
    -m 4096 \
    -cpu host \
    -enable-kvm \
    -cdrom khazaros.iso \
    -boot d

# Or create a persistent disk
qemu-img create -f qcow2 khazaros.qcow2 20G
qemu-system-x86_64 \
    -m 4096 \
    -cpu host \
    -enable-kvm \
    -hda khazaros.qcow2 \
    -cdrom khazaros.iso \
    -boot d
```

After boot, the live environment starts. Run:

```bash
kha "open firefox"
kha status
```

## Option B: VirtualBox

Free and cross-platform (Windows, macOS, Linux).

```bash
# Install VirtualBox
sudo dnf install VirtualBox

# Create VM:
#  - Type: Linux
#  - Version: Fedora (64-bit)
#  - RAM: 4096 MB
#  - Disk: 20 GB (dynamically allocated)
#  - Network: NAT
#  - Enable EFI in Settings -> System

# Attach the ISO and boot
```

### VirtualBox Settings

| Setting | Value |
|---------|-------|
| RAM | 4096 MB minimum |
| CPU | 2+ cores |
| Video Memory | 128 MB |
| Graphics Controller | VMSVGA |
| Acceleration | Enable VT-x/AMD-V, Nested Paging |
| Network | NAT (or Bridged for agent testing) |
| Storage | 20 GB dynamic |

## Option C: Physical Hardware

Full installation on real hardware.

```bash
# Write ISO to USB (replace sdX with your device)
sudo dd if=khazaros.iso of=/dev/sdX bs=4M status=progress
sync

# Boot from USB (F12/F2/Del during POST to select boot device)
# Select "Install KhazarOS" or "Try KhazarOS"
```

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU | x86_64, 2 cores | 4+ cores |
| RAM | 4 GB | 8 GB |
| Disk | 20 GB | 40 GB SSD |
| GPU | Any with modesetting | Intel/AMD for acceleration |
| Network | Ethernet or WiFi | WiFi for network-agent testing |
| Audio | Any ALSA device | PulseAudio/PipeWire for audio-agent |

## Testing the AI Pipeline

Once KhazarOS is booted, test the full pipeline:

```bash
# 1. Check all services
kha status

# 2. Tier 0 intent resolution
kha "open firefox"

# 3. Package management
kha "search vlc"
kha "update system"

# 4. Network control
kha "turn on wifi"
kha "network status"

# 5. Audio control
kha "volume up"

# 6. Power (policy check)
kha "shutdown"

# 7. Check daemon logs
journalctl -u khazar.target -f
journalctl -u ai-orchestrator -n 20
```

## Troubleshooting

### Service not starting

```bash
sudo systemctl status ai-orchestrator
sudo journalctl -u ai-orchestrator -n 50 --no-pager
```

### Agent not registering

```bash
ls -la /run/khazar/*.sock
cat /etc/khazar/orchestrator.toml
```

### No intent matched

```bash
echo '{"id":1,"type":"request","query":"open firefox"}' | socat - UNIX-CONNECT:/run/khazar/rule-engine.sock
```

### Policy denies everything

Check the policy rules at `/etc/khazar/policies/default.policy.toml`. The default policy allows common operations but may block system-critical ones.

## Building from Source (for Developers)

If you want to build KhazarOS yourself instead of using the ISO:

```bash
git clone https://github.com/Khazar-System-Distribution/khazar
cd khazar
make all                         # Build all components
make -C sdk test                 # Run SDK tests

# For distro image build
make -C distro build             # Requires podman + bootc

# Manual install on existing Fedora
sudo bash distro/install.sh
sudo systemctl enable khazar.target --now
```
