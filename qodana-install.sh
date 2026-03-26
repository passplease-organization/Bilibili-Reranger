#!/bin/bash
# scripts/qodana-bootstrap.sh
# ✅ 此脚本在 Qodana 容器内执行，负责安装所有依赖
set -euo pipefail

echo "🚀 Qodana bootstrap starting..."

# 1️⃣ 配置 vcpkg 环境变量
export VCPKG_ROOT="/opt/vcpkg"
export PATH="$VCPKG_ROOT:$PATH"

# 2️⃣ 安装 vcpkg（如果不存在）
if [ ! -d "$VCPKG_ROOT" ]; then
  echo "📥 Cloning vcpkg..."
  git clone --depth=1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
  cd "$VCPKG_ROOT"
  ./bootstrap-vcpkg.sh -disableMetrics
  ./vcpkg integrate install
fi

# 3️⃣ 启用 vcpkg binary caching（加速后续运行）
export VCPKG_BINARY_SOURCES="clear;files,$PWD/.qodana/vcpkg-cache,readwrite"
mkdir -p "$PWD/.qodana/vcpkg-cache"

# 4️⃣ 安装系统依赖（静默输出）
echo "📦 Installing system dependencies..."
apt-get update -qq
apt-get install -y -qq \
  libboost-all-dev \
  bison flex \
  autoconf automake libtool pkg-config autoconf-archive \
  > /dev/null 2>&1 || true

# 5️⃣ 安装 vcpkg 包（您的依赖列表）
echo "📦 Installing vcpkg packages..."
"$VCPKG_ROOT/vcpkg" install \
  libpqxx \
  nlohmann-json \
  cpr \
  curl \
  openssl \
  libsodium \
  || echo "⚠️ vcpkg install failed, continuing"

# 6️⃣ 生成 compile_commands.json（C++ 分析必需！）
echo "🔧 Generating compile_commands.json..."
if [ -f "CMakeLists.txt" ] && [ ! -f "build/compile_commands.json" ]; then
  cmake -S . -B build \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    > /dev/null 2>&1 || echo "⚠️ CMake configure failed, continuing"
fi

echo "✅ Bootstrap completed successfully"