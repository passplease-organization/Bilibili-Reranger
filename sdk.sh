#!/usr/bin/env bash
set -euo pipefail

SDK_PREFIX="/opt/bilibili-reranger-sdk/debug"
LOCAL_SDK_DIR="./.devcontainer/sdk"

cmake --preset debug
cmake --build --preset debug

sudo rm -rf "${SDK_PREFIX}"
sudo cmake --install build/debug --prefix "${SDK_PREFIX}"

sudo rm -rf "${LOCAL_SDK_DIR}"
sudo mkdir -p "${LOCAL_SDK_DIR}"
sudo cp -a "${SDK_PREFIX}/." "${LOCAL_SDK_DIR}/"
