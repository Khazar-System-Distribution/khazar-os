# Allow build scripts to be referenced without being copied into the final image
FROM scratch AS ctx
COPY build_files /
COPY system_files /system_files
COPY khazar /khazar

# Base Image — Bluefin (GNOME + developer tooling)
FROM ghcr.io/ublue-os/bluefin:stable

### MODIFICATIONS
# build.sh handles: Khazar platform build + Ollama install + system overlay + systemd enable
RUN --mount=type=bind,from=ctx,source=/,target=/ctx \
    --mount=type=cache,dst=/var/cache \
    --mount=type=cache,dst=/var/log \
    --mount=type=tmpfs,dst=/tmp \
    /ctx/build.sh

### LINTING
RUN bootc container lint
