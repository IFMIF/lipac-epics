#!/bin/bash

# Abort on failure
set -e

# Variables
OUTPUT_PACKAGE="${CI_PROJECT_NAME}-linux-x86_64.tar.xz"

# Pretty printing of information messages
function red_text() {
    echo -e "\033[31;1m"
    echo "--------------------------------------------------------------------------------"
    echo "$1"
    echo "--------------------------------------------------------------------------------"
    echo -e "\033[0m"
}

# Pretty printing of information messages
function green_text() {
    echo -e "\033[32;1m"
    echo "$1"
    echo -e "\033[0m"
}

# -----------------------------------------------------------------------------
# Go
# -----------------------------------------------------------------------------

# Build and push the OCI image
red_text "Building the OCI image ${CI_REGISTRY_IMAGE}"
podman build --tag "${CI_REGISTRY_IMAGE}:latest" .
podman push "${CI_REGISTRY_IMAGE}:latest"

# Extract the build artifacts and build a tar.xz package
red_text "Build the output package ${OUTPUT_PACKAGE}"
mkdir -p /build-results/
podman create --name tmp-epics "$CI_REGISTRY_IMAGE:latest"
podman cp tmp-epics:/opt /build-results
podman rm tmp-epics
date +'%Y.%m.%d' > /build-results/BUILD_DATE
tar -C /build-results/ -cJf "${OUTPUT_PACKAGE}" .

# Upload the package to the mirror server
red_text "Package will be uploaded to ${UPLOAD_URL}"
status=$(curl -s -w '%{http_code}' -o /dev/null --location --upload-file "${OUTPUT_PACKAGE}" "${UPLOAD_URL}")
if [[ "$status" != "201" ]]; then
    echo "Expected HTTP 201 Created, got $status" >&2
    exit 1
fi
echo "OK"

# Done
red_text "Finished"
