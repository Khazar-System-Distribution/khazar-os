FROM scratch AS ctx
COPY build_files /
COPY system_files /system_files

# Base: Fedora 40 bootc (Silverblue)
FROM quay.io/fedora/fedora-bootc:40

RUN --mount=type=bind,from=ctx,source=/,target=/ctx \
    --mount=type=cache,dst=/var/cache \
    --mount=type=cache,dst=/var/log \
    --mount=type=tmpfs,dst=/tmp \
    /ctx/build.sh

RUN bootc container lint
