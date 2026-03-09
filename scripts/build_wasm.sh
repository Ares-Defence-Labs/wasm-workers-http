#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "==> Repo root:"
echo "   $ROOT"

cd "$ROOT"

echo "==> Running bootstrap"
bash "$ROOT/scripts/bootstrap.sh"

echo "==> Removing old wasm debug build dir"
rm -rf "$ROOT/build-wasm-debug"

echo "==> Listing available presets"
cmake --list-presets

echo "==> Configuring wasm-debug"
cmake --preset wasm-debug

echo "==> Building build-wasm-debug"
cmake --build --preset build-wasm-debug

echo ""
echo "WASM debug build complete"