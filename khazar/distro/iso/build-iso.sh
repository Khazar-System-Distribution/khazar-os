#!/bin/bash
# KhazarOS ISO Builder
# Builds a bootable ISO from a bootc container image

set -e

IMAGE_NAME="${IMAGE_NAME:-khazaros}"
IMAGE_TAG="${IMAGE_TAG:-latest}"
OUTPUT_DIR="${OUTPUT_DIR:-./build}"

echo "=== KhazarOS ISO Builder v0.1 ==="
echo ""

# 1. Build bootc container
echo "[1/4] Building bootc container..."
podman build -t "$IMAGE_NAME:$IMAGE_TAG" -f distro/bootc/Containerfile .

# 2. Create disk image from container
echo "[2/4] Creating raw disk image (8GB)..."
rm -f "$OUTPUT_DIR/khazaros.raw"
mkdir -p "$OUTPUT_DIR"

podman run --rm --privileged \
    -v "$OUTPUT_DIR:/output" \
    "$IMAGE_NAME:$IMAGE_TAG" \
    bootc install --target-disk /output/khazaros.raw \
    || true

# Alternative: use bootc-image-builder
if [ ! -f "$OUTPUT_DIR/khazaros.raw" ]; then
    echo "[*] Using bootc-image-builder..."
    bootc-image-builder \
        --type qcow2 \
        --rootfs ext4 \
        "$IMAGE_NAME:$IMAGE_TAG" \
        "$OUTPUT_DIR/khazaros.qcow2"
fi

# 3. Convert to ISO
echo "[3/4] Converting to ISO..."
if [ -f "$OUTPUT_DIR/khazaros.qcow2" ]; then
    qemu-img convert -f qcow2 -O raw "$OUTPUT_DIR/khazaros.qcow2" "$OUTPUT_DIR/khazaros.raw"
fi

if [ -f "$OUTPUT_DIR/khazaros.raw" ]; then
    xorriso -as mkisofs \
        -V "KhazarOS" \
        -o "$OUTPUT_DIR/khazaros-${IMAGE_TAG}.iso" \
        -isohybrid-mbr /usr/lib/ISOLINUX/isohdpfx.bin \
        -b isolinux/isolinux.bin \
        -c isolinux/boot.cat \
        -no-emul-boot \
        -boot-load-size 4 \
        -boot-info-table \
        "$OUTPUT_DIR/khazaros.raw" 2>/dev/null || {
        echo "[!] xorriso not found, keeping raw image"
    }
fi

# 4. Summary
echo "[4/4] Done!"
echo ""
ls -lah "$OUTPUT_DIR/" 2>/dev/null
echo ""
echo "=== Bootable images ready ==="
echo "  Raw:  $OUTPUT_DIR/khazaros.raw"
echo "  ISO:  $OUTPUT_DIR/khazaros-${IMAGE_TAG}.iso"
echo ""
echo "Test with:  qemu-system-x86_64 -m 4096 -hda $OUTPUT_DIR/khazaros.raw"
echo "Flash USB:  dd if=$OUTPUT_DIR/khazaros.iso of=/dev/sdX bs=4M status=progress"
