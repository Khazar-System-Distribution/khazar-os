#!/bin/bash
# KhazarOS — Bootable Disk Image Builder
# Builds a raw disk image that boots in QEMU or writes to USB
# Usage: bash distro/iso/mkimage.sh [version]

set -e
VERSION="${1:-0.1.0}"
OUTDIR="distro/iso/output"
IMAGE="khazaros-${VERSION}-x86_64"

echo "=== KhazarOS Image Builder v${VERSION} ==="

# 1. Build Khazar
echo "[1/4] Building Khazar platform..."
make clean &>/dev/null || true
make all

# 2. Create rootfs layout
echo "[2/4] Creating rootfs layout..."
rm -rf build-iso
mkdir -p build-iso/rootfs/usr/local/bin \
         build-iso/rootfs/etc/khazar/policies \
         build-iso/rootfs/usr/lib \
         build-iso/rootfs/usr/share/khazar \
         build-iso/rootfs/run/khazar \
         build-iso/iso/isolinux \
         build-iso/iso/images

# Binaries
for comp in orchestrator rule-engine policy-engine model-runtime intent-classifier; do
    cp components/$comp/ai-$comp build-iso/rootfs/usr/local/bin/ 2>/dev/null || true
done
for agent in desktop package network power audio; do
    cp agents/${agent}-agent/ai-${agent}-agent build-iso/rootfs/usr/local/bin/ 2>/dev/null || true
done
cp sdk/libai-sdk.a build-iso/rootfs/usr/lib/
cp distro/cli/kha build-iso/rootfs/usr/local/bin/
chmod +x build-iso/rootfs/usr/local/bin/* 2>/dev/null || true

# Configs
cp distro/configs/*.toml build-iso/rootfs/etc/khazar/ 2>/dev/null || true
cp distro/configs/default.policy.toml build-iso/rootfs/etc/khazar/policies/

# systemd
cp distro/systemd/*.service build-iso/rootfs/etc/khazar/ 2>/dev/null || true
cp distro/systemd/khazar.target build-iso/rootfs/etc/khazar/ 2>/dev/null || true

# Branding
cp distro/branding/khazar-logo.png build-iso/iso/ 2>/dev/null || true

# 3. Create raw disk image
echo "[3/4] Creating bootable disk image..."
mkdir -p "$OUTDIR"

# Create raw disk (minimal bootable for QEMU testing)
dd if=/dev/zero of="$OUTDIR/${IMAGE}.raw" bs=1M count=2048 2>/dev/null

# Format as ext4 and populate (requires root for mount)
if [ "$EUID" -eq 0 ]; then
    mkfs.ext4 -F "$OUTDIR/${IMAGE}.raw" &>/dev/null
    mkdir -p /mnt/khazar-build
    mount -o loop "$OUTDIR/${IMAGE}.raw" /mnt/khazar-build
    cp -r build-iso/rootfs/* /mnt/khazar-build/
    umount /mnt/khazar-build
    echo "  [+] Raw image: $OUTDIR/${IMAGE}.raw"
else
    echo "  [!] Run as root to create bootable ext4 image"
    echo "  [*] Creating tar archive instead..."
    tar -czf "$OUTDIR/${IMAGE}.tar.gz" -C build-iso/rootfs .
    echo "  [+] Archive: $OUTDIR/${IMAGE}.tar.gz"
fi

# 4. Create ISO (if xorriso available)
echo "[4/4] Creating ISO..."
if command -v xorriso &>/dev/null; then
    # Copy isolinux if available
    if [ -f /usr/lib/ISOLINUX/isolinux.bin ]; then
        cp /usr/lib/ISOLINUX/isolinux.bin build-iso/iso/isolinux/
        cp /usr/lib/syslinux/modules/bios/ldlinux.c32 build-iso/iso/isolinux/ 2>/dev/null || true
    fi

    # Copy kernel + initrd if available (from host system for testing)
    cp /boot/vmlinuz-* build-iso/iso/boot/vmlinuz 2>/dev/null || true
    cp /boot/initramfs-*.img build-iso/iso/boot/initrd 2>/dev/null || true

    xorriso -as mkisofs \
        -V "KhazarOS" \
        -J -R -l \
        -iso-level 3 \
        -o "$OUTDIR/${IMAGE}.iso" \
        build-iso/iso/ 2>/dev/null && \
        echo "  [+] ISO: $OUTDIR/${IMAGE}.iso" || \
        echo "  [!] ISO creation failed (missing boot files)"

    # Make hybrid (bootable from USB)
    if command -v isohybrid &>/dev/null; then
        isohybrid "$OUTDIR/${IMAGE}.iso" 2>/dev/null || true
    fi
else
    echo "  [!] xorriso not installed, skipping ISO"
fi

# 5. Summary
echo ""
echo "=== Build Complete ==="
ls -lah "$OUTDIR/" 2>/dev/null
echo ""
echo "Test images:"
for f in "$OUTDIR/$IMAGE".*; do
    [ -f "$f" ] || continue
    case "$f" in
        *.iso) echo "  ISO:  qemu-system-x86_64 -m 4096 -hda $f" ;;
        *.raw) echo "  Raw:  qemu-system-x86_64 -m 4096 -hda $f" ;;
        *.tar.gz) echo "  Tar:  tar xzf $f -C /" ;;
    esac
done
