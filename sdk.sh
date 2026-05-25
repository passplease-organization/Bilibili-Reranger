#!/usr/bin/env bash
sudo rm -r -f /opt/bilibili-reranger-sdk/debug/.
sudo cmake --install cmake-build-debug --prefix /opt/bilibili-reranger-sdk/debug
sudo cp -a /opt/bilibili-reranger-sdk/debug/. ./.devcontainer/sdk/
