# KhazarOS Distro Build
# Builds a bootable OS image with Khazar AI pre-installed

IMAGE   ?= khazaros
TAG     ?= 0.1.0
ARCH    ?= x86_64

.PHONY: build iso test-qemu flash release clean

build:
	@echo "=== Building KhazarOS container (bootc) ==="
	podman build --tag $(IMAGE):$(TAG) -f distro/bootc/Containerfile .

iso:
	@bash distro/iso/mkimage.sh $(TAG)

test-qemu: iso
	@echo "=== Testing in QEMU ==="
	@qemu-system-x86_64 -m 4096 -cpu host -enable-kvm \
		-hda distro/iso/output/khazaros-$(TAG)-x86_64.iso 2>/dev/null || \
	qemu-system-x86_64 -m 4096 \
		-hda distro/iso/output/khazaros-$(TAG)-x86_64.raw 2>/dev/null || \
	echo "No bootable image found. Run 'make iso' first."

flash:
	@echo "=== Flash to USB ==="
	@if [ -z "$(DISK)" ]; then \
		echo "Usage: make flash DISK=/dev/sdX"; \
		exit 1; \
	fi
	@test -f distro/iso/output/khazaros.iso || (echo "Run 'make build-iso' first" && exit 1)
	dd if=distro/iso/output/khazaros.iso of=$(DISK) bs=4M status=progress
	sync

release:
	@echo "=== Building release $(TAG) ==="
	@bash distro/iso/build.sh
	@cp distro/iso/output/khazaros.iso "khazaros-$(TAG)-$(ARCH).iso"
	@echo "Release: khazaros-$(TAG)-$(ARCH).iso"

clean:
	rm -rf distro/iso/output
	podman rmi $(IMAGE):$(TAG) 2>/dev/null || true
