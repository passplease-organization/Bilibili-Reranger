#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="${SCRIPT_DIR}/db_probe.cpp"
OUT="${TMPDIR:-/tmp}/backend_postgres_db_probe"

VCPKG_ROOT="${VCPKG_ROOT:-/usr/local/vcpkg}"
TRIPLET="${VCPKG_DEFAULT_TRIPLET:-x64-linux}"
INCLUDE_DIR="${VCPKG_ROOT}/installed/${TRIPLET}/include"
LIB_DIR="${VCPKG_ROOT}/installed/${TRIPLET}/lib"

if [[ ! -d "${INCLUDE_DIR}/pqxx" || ! -f "${LIB_DIR}/libpqxx.a" ]]; then
  echo "Could not find pqxx under ${VCPKG_ROOT}/installed/${TRIPLET}" >&2
  echo "Set VCPKG_ROOT or VCPKG_DEFAULT_TRIPLET, or install the repo dependencies first." >&2
  exit 1
fi

"${CXX:-c++}" -std=c++23 "${SRC}" \
  -I"${INCLUDE_DIR}" \
  -L"${LIB_DIR}" \
  -lpqxx -lpq -lpgcommon -lpgport -lssl -lcrypto -lz -llz4 \
  -o "${OUT}"

"${OUT}" "$@"
