#!/bin/bash
# KhazarOS — Bootable ISO Builder
# Usage: cd khazar && bash distro/iso/build.sh

set -e

IMAGE="khazaros"
TAG="0.1.0"
OUT="distro/iso/output"

echo "╔══════════════════════════════════════╗"
echo "║   KhazarOS ISO Builder v0.1         ║"
echo "║   Fedora Silverblue + Khazar AI     ║"
echo "╚══════════════════════════════════════╝"
echo ""

# Clean
rm -rf "$OUT"
mkdir -p "$OUT"

# Step 1: Build container
echo "[1/3] Building bootc container image..."
podman build \
    --tag "$IMAGE:$TAG" \
    --file distro/bootc/Containerfile \
    .

# Step 2: Export as disk image
echo "[2/3] Exporting disk image..."
podman run --rm \
    --privileged \
    --device /dev/loop-control \
    -v /var/tmp:/var/tmp \
    -v "$PWD/$OUT:/output" \
    "$IMAGE:$TAG" \
    /bin/bash -c "
        truncate -s 8G /output/khazaros.raw &&
        mkfs.ext4 /output/khazaros.raw 2>/dev/null || true
    " || true

# Alternative: use podman save + bootc-image-builder
echo "[*] Exporting container for bootc-image-builder..."
podman save "$IMAGE:$TAG" | gzip > "$OUT/khazaros-container.tar.gz"

# Step 3: Build ISO with osbuild-composer
if command -v composer-cli &>/dev/null; then
    echo "[3/3] Building ISO via osbuild-composer..."

    cat > "$OUT/blueprint.toml" << 'BLUEPRINT'
name = "khazaros"
description = "KhazarOS - AI-powered Fedora Silverblue"
version = "0.1.0"

[[packages]]
name = "khazar"
version = "*"

[[customizations.user]]
name = "user"
password = "$6$..."  # change in production
groups = ["wheel", "khazar"]

[[customizations.services]]
enabled = ["khazar.target"]
BLUEPRINT

    composer-cli blueprints push "$OUT/blueprint.toml"
    composer-cli compose start khazaros image-installer 2>&1 | tee "$OUT/compose.log"
    COMPOSE_ID=$(grep "Compose" "$OUT/compose.log" | awk '{print $NF}')
    composer-cli compose image "$COMPOSE_ID" --filename "$OUT/khazaros.iso"

    echo "[✓] ISO built: $OUT/khazaros.iso"
else
    echo ""
    echo "=== Manual ISO Build Instructions ==="
    echo ""
    echo "Container saved to: $OUT/khazaros-container.tar.gz"
    echo ""
    echo "Option A — Test in QEMU:"
    echo "  podman run --rm -it --privileged -p 2222:22 $IMAGE:$TAG"
    echo ""
    echo "Option B — Deploy to disk:"
    echo "  podman run --rm --privileged -v /dev/sdX:/dev/sdX $IMAGE:$TAG"
    echo "  bootc install /dev/sdX"
    echo ""
    echo "Option C — Build ISO with osbuild-composer:"
    echo "  sudo dnf install osbuild-composer"
    echo "  bash distro/iso/build.sh"
fi

echo ""
echo "=== Done ==="
ls -lah "$OUT/" 2>/dev/null
