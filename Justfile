set dotenv-filename := "image-template.env"
set dotenv-load

export image_name := env_var("IMAGE_NAME")
export repo_organization := env_var("REPO_ORGANIZATION")
export image_desc := env_var("IMAGE_DESC")
export image_keywords := env_var("IMAGE_KEYWORDS")
export image_logo_url := env_var("IMAGE_LOGO_URL")
export default_tag := env_var("DEFAULT_TAG")
export bib_image := env_var("BIB_IMAGE")

alias build-vm := build-qcow2

[private]
default:
    @just --list

# Check Just Syntax
[group('Just')]
check:
    #!/usr/bin/bash
    find . -type f -name "*.just" | while read -r file; do
    	echo "Checking syntax: $file"
    	just --unstable --fmt --check -f $file
    done
    echo "Checking syntax: Justfile"
    just --unstable --fmt --check -f Justfile

# Fix Just Syntax
[group('Just')]
fix:
    #!/usr/bin/bash
    find . -type f -name "*.just" | while read -r file; do
    	echo "Checking syntax: $file"
    	just --unstable --fmt -f $file
    done
    echo "Checking syntax: Justfile"
    just --unstable --fmt -f Justfile || { exit 1; }

# Clean Repo
[group('Utility')]
clean:
    #!/usr/bin/bash
    set -eoux pipefail
    touch _build
    find *_build* -exec rm -rf {} \;
    rm -f previous.manifest.json changelog.md output.env
    rm -rf output/

# Build the image
build $target_image=image_name $tag=default_tag:
    #!/usr/bin/env bash
    set -euox pipefail

    BUILD_ARGS=()
    LABELS=()
    if [[ -z "$(git status -s)" ]]; then
        GIT_SHA=$(git rev-parse --short HEAD)
        LABELS+=("--label" "io.artifacthub.package.readme-url=https://raw.githubusercontent.com/{{ repo_organization }}/{{ image_name }}/${GIT_SHA}/README.md")
        LABELS+=("--label" "org.opencontainers.image.documentation=https://raw.githubusercontent.com/{{ repo_organization }}/{{ image_name }}/${GIT_SHA}/README.md")
        LABELS+=("--label" "org.opencontainers.image.source=https://github.com/{{ repo_organization }}/{{ image_name }}/blob/${GIT_SHA}/Containerfile")
        LABELS+=("--label" "org.opencontainers.image.url=https://github.com/{{ repo_organization }}/{{ image_name }}/tree/${GIT_SHA}")
        LABELS+=("--label" "org.opencontainers.image.version={{ default_tag }}.$(date +%Y%m%d)-${GIT_SHA}")
    fi

    LABELS+=("--label" "io.artifacthub.package.deprecated=false")
    LABELS+=("--label" "io.artifacthub.package.keywords={{ image_keywords }}")
    LABELS+=("--label" "io.artifacthub.package.license=Apache-2.0")
    LABELS+=("--label" "io.artifacthub.package.logo-url={{ image_logo_url }}")
    LABELS+=("--label" "io.artifacthub.package.prerelease=false")
    LABELS+=("--label" "org.opencontainers.image.created=$(date -u +%Y\-%m\-%d\T%H\:%M\:%S\Z)")
    LABELS+=("--label" "org.opencontainers.image.description={{ image_desc }}")
    LABELS+=("--label" "org.opencontainers.image.title={{ image_name }}")
    LABELS+=("--label" "org.opencontainers.image.vendor={{ repo_organization }}")

    PODMAN_BUILD_ARGS=("${BUILD_ARGS[@]}" "${LABELS[@]}" --pull=newer --tag "${target_image}:${tag}" --file Containerfile)
    podman build "${PODMAN_BUILD_ARGS[@]}" .

# Rechunk with rpm-ostree
ostree-rechunk $target_image=image_name $tag=default_tag:
    #!/usr/bin/env bash
    set -xeuo pipefail
    if [[ ! "${UID}" -eq "0" ]]; then
      echo "This needs to run as root."
      exit 1
    fi
    RPM_OSTREE_CHUNKER_IMAGE="quay.io/fedora/fedora-bootc:latest"
    podman run --rm \
      --pull=newer \
      --privileged \
      -v "/var/lib/containers:/var/lib/containers" \
      --entrypoint /usr/bin/rpm-ostree \
      "${RPM_OSTREE_CHUNKER_IMAGE}" \
      compose build-chunked-oci \
      --max-layers 127 \
      --format-version=2 \
      --bootc \
      --from "localhost/${target_image}:${tag}" \
      --output containers-storage:"localhost/${target_image}:${tag}"

# Generate Default Tag
[group('Utility')]
generate-default-tag $tag=default_tag:
    #!/usr/bin/env bash
    set -eoux pipefail
    echo "${tag}"

# Generate Build Tags
[group('Utility')]
generate-build-tags $target_image=image_name $tag=default_tag:
    #!/usr/bin/bash
    set -eoux pipefail
    DATE=$(date +%Y%m%d)
    BUILD_TAGS=()
    if [[ -z "$(git status -s)" ]]; then
        GIT_SHA=$(git rev-parse --short HEAD)
        BUILD_TAGS+=("${tag}-${GIT_SHA}")
        BUILD_TAGS+=("${tag}-${DATE}-${GIT_SHA}")
        BUILD_TAGS+=("${DATE}-${GIT_SHA}")
    fi
    BUILD_TAGS+=("${DATE}")
    BUILD_TAGS+=("${tag}")
    BUILD_TAGS+=("${tag}-${DATE}")
    echo "${BUILD_TAGS[@]}"

# Tag Images
[group('Utility')]
tag-images $target_image=image_name $tag=default_tag tags="":
    #!/usr/bin/env bash
    set -eoux pipefail
    IMAGE=$(podman inspect ${target_image}:${tag} | jq -r .[].Id)
    podman untag ${IMAGE}
    for tag in {{ tags }}; do
        podman tag $IMAGE "${target_image}:${tag}"
    done
    podman images

# Image Name
[group('Utility')]
[private]
image_name $target_image=image_name:
    #!/usr/bin/env bash
    set -eoux pipefail
    echo "${image_name}"

# Lint all bash scripts
lint:
    #!/usr/bin/env bash
    set -eoux pipefail
    if ! command -v shellcheck &> /dev/null; then
        echo "shellcheck could not be found. Please install it."
        exit 1
    fi
    /usr/bin/find . -iname "*.sh" -type f -exec shellcheck "{}" ';'

# Format all bash scripts
format:
    #!/usr/bin/env bash
    set -eoux pipefail
    if ! command -v shfmt &> /dev/null; then
        echo "shfmt could not be found. Please install it."
        exit 1
    fi
    /usr/bin/find . -iname "*.sh" -type f -exec shfmt --write "{}" ';'

# Build QCOW2
build-qcow2 $target_image=("localhost/" + image_name) $tag=default_tag:
    #!/usr/bin/env bash
    set -euo pipefail
    BUILDTMP=$(mktemp -p "${PWD}" -d -t _build-bib.XXXXXXXXXX)
    sudo podman run \
      --rm \
      -it \
      --privileged \
      --pull=newer \
      --net=host \
      --security-opt label=type:unconfined_t \
      -v $(pwd)/disk_config/disk.toml:/config.toml:ro \
      -v $BUILDTMP:/output \
      -v /var/lib/containers/storage:/var/lib/containers/storage \
      "${bib_image}" \
      --type qcow2 \
      --use-librepo=True \
      --rootfs=btrfs \
      "${target_image}:${tag}"
    mkdir -p output
    sudo mv -f $BUILDTMP/* output/
    sudo rmdir $BUILDTMP
    sudo chown -R $USER:$USER output/

# Build ISO locally
build-iso $target_image=("localhost/" + image_name) $tag=default_tag:
    #!/usr/bin/env bash
    set -euo pipefail
    BUILDTMP=$(mktemp -p "${PWD}" -d -t _build-bib.XXXXXXXXXX)
    sudo podman run \
      --rm \
      -it \
      --privileged \
      --pull=newer \
      --net=host \
      --security-opt label=type:unconfined_t \
      -v $(pwd)/disk_config/iso.toml:/config.toml:ro \
      -v $BUILDTMP:/output \
      -v /var/lib/containers/storage:/var/lib/containers/storage \
      "${bib_image}" \
      --type iso \
      --use-librepo=True \
      --rootfs=btrfs \
      "${target_image}:${tag}"
    mkdir -p output
    sudo mv -f $BUILDTMP/* output/
    sudo rmdir $BUILDTMP
    sudo chown -R $USER:$USER output/
