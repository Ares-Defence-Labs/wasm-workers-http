#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_ROOT="$ROOT/vcpkg"

export VCPKG_DISABLE_METRICS=1

echo "==> Checking prerequisites"
command -v git >/dev/null 2>&1 || { echo "❌ git not found"; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "❌ cmake not found"; exit 1; }
command -v node >/dev/null 2>&1 || { echo "❌ node not found"; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "❌ python3 not found"; exit 1; }

if ! command -v emcc >/dev/null 2>&1; then
  echo "❌ Emscripten not installed (emcc not found)."
  exit 1
fi

echo "==> Updating git submodules (if any)"
git -C "$ROOT" submodule update --init --recursive || true

echo "==> Ensuring vcpkg repository exists"
if [ ! -f "$VCPKG_ROOT/bootstrap-vcpkg.sh" ]; then
  echo "   cloning vcpkg..."
  rm -rf "$VCPKG_ROOT"
  git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT"
fi

echo "==> Bootstrapping vcpkg"
if [ ! -x "$VCPKG_ROOT/vcpkg" ]; then
  (cd "$VCPKG_ROOT" && ./bootstrap-vcpkg.sh -disableMetrics)
fi

echo "==> Locating Emscripten toolchain"
EMCC_REAL="$(python3 - <<'PY'
import os, shutil
p = shutil.which("emcc")
print(os.path.realpath(p) if p else "")
PY
)"

if [ -z "$EMCC_REAL" ]; then
  echo "❌ Could not resolve emcc"
  exit 1
fi

EM_PREFIX="$(cd "$(dirname "$EMCC_REAL")/.." && pwd)"

CANDIDATES=(
  "$EM_PREFIX/libexec/cmake/Modules/Platform/Emscripten.cmake"
  "$EM_PREFIX/libexec/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
  "$EM_PREFIX/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
  "$EM_PREFIX/cmake/Modules/Platform/Emscripten.cmake"
)

EMSCRIPTEN_TOOLCHAIN_FILE=""
for p in "${CANDIDATES[@]}"; do
  if [ -f "$p" ]; then
    EMSCRIPTEN_TOOLCHAIN_FILE="$p"
    break
  fi
done

if [ -z "$EMSCRIPTEN_TOOLCHAIN_FILE" ]; then
  FOUND="$(find "$EM_PREFIX" -maxdepth 8 -type f -name "Emscripten.cmake" 2>/dev/null | head -n 1 || true)"
  if [ -n "${FOUND:-}" ]; then
    EMSCRIPTEN_TOOLCHAIN_FILE="$FOUND"
  fi
fi

if [ -z "$EMSCRIPTEN_TOOLCHAIN_FILE" ]; then
  echo "❌ Could not find Emscripten.cmake"
  exit 1
fi

EMSCRIPTEN_ROOT_PATH="$(cd "$(dirname "$EMSCRIPTEN_TOOLCHAIN_FILE")/../../.." && pwd)"

export EMSCRIPTEN_ROOT_PATH
export EMSCRIPTEN_TOOLCHAIN_FILE
export VCPKG_ROOT

echo "==> emcc resolved to:"
echo "   $EMCC_REAL"
echo "==> Using EMSCRIPTEN_TOOLCHAIN_FILE:"
echo "   $EMSCRIPTEN_TOOLCHAIN_FILE"

MANIFEST="$ROOT/cpp/vcpkg.json"
OVERLAY_TRIPLETS="$ROOT/cpp/vcpkg-triplets"

if [ ! -f "$MANIFEST" ]; then
  echo "❌ Missing vcpkg manifest: $MANIFEST"
  exit 1
fi

if [ ! -f "$OVERLAY_TRIPLETS/wasm32-emscripten.cmake" ]; then
  echo "❌ Missing overlay triplet: $OVERLAY_TRIPLETS/wasm32-emscripten.cmake"
  exit 1
fi

echo "==> Installing vcpkg deps"
"$VCPKG_ROOT/vcpkg" install \
  --x-manifest-root="$ROOT/cpp" \
  --x-install-root="$ROOT/cpp/vcpkg_installed" \
  --overlay-triplets="$OVERLAY_TRIPLETS" \
  --triplet wasm32-emscripten \
  --disable-metrics

if [ -d "$ROOT/cloudflare" ]; then
  echo "==> Installing worker dependencies"
  cd "$ROOT/cloudflare" &&
  npm install
fi

if [ -d "$ROOT/cloudflare/packages/aresWasmWorkerRuntime" ]; then
  echo "==> Installing worker dependencies of http runtime"
  cd "$ROOT/cloudflare/packages/aresWasmWorkerRuntime" &&
  npm install &&
  npm run build
fi

echo "Bootstrap complete"