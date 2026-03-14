#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_ROOT="$ROOT/vcpkg"
EMSDK_ROOT="$ROOT/emsdk"
EMSDK_VERSION="${EMSDK_VERSION:-4.0.21}"

VCPKG_MANIFEST_ROOT="$ROOT/cpp"
VCPKG_INSTALL_ROOT="$ROOT/cpp/vcpkg_installed"
VCPKG_OVERLAY_TRIPLETS="$ROOT/cpp/vcpkg-triplets"
VCPKG_TRIPLET="wasm32-emscripten"

LOCAL_PRESETS_FILE="$ROOT/cmake-local-presets.json"
LOCAL_ENV_FILE="$ROOT/scripts/emscripten-env.sh"

export VCPKG_DISABLE_METRICS=1

log() {
  printf '==> %s\n' "$1"
}

fail() {
  printf '❌ %s\n' "$1" >&2
  exit 1
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || fail "$1 not found"
}

write_local_presets() {
  local toolchain_file="$1"
  local em_config_file="$2"

  cat > "$LOCAL_PRESETS_FILE" <<EOF
{
  "version": 4,
  "configurePresets": [
    {
      "name": "wasm-local",
      "hidden": true,
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "VCPKG_CHAINLOAD_TOOLCHAIN_FILE": "${toolchain_file}"
      },
      "environment": {
        "EM_CONFIG": "${em_config_file}"
      }
    }
  ]
}
EOF
}

write_local_env() {
  local emsdk_env_script="$1"
  local em_config_file="$2"
  local em_cache_dir="$3"

  cat > "$LOCAL_ENV_FILE" <<EOF
#!/usr/bin/env bash
set -euo pipefail

ROOT="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")/.." && pwd)"
source "${emsdk_env_script}"

export EM_CONFIG="${em_config_file}"
export EM_CACHE="${em_cache_dir}"
export VCPKG_ROOT="${VCPKG_ROOT}"
EOF

  chmod +x "$LOCAL_ENV_FILE"
}

log "Checking prerequisites"
require_cmd git
require_cmd cmake
require_cmd python3
require_cmd node

log "Updating git submodules (if any)"
git -C "$ROOT" submodule update --init --recursive || true

log "Ensuring local emsdk exists"
if [ ! -d "$EMSDK_ROOT/.git" ]; then
  git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_ROOT"
fi

log "Installing local Emscripten SDK version: $EMSDK_VERSION"
(
  cd "$EMSDK_ROOT"
  ./emsdk install "$EMSDK_VERSION"
  ./emsdk activate "$EMSDK_VERSION"
)

EMSDK_ENV_SCRIPT="$EMSDK_ROOT/emsdk_env.sh"
[ -f "$EMSDK_ENV_SCRIPT" ] || fail "Missing emsdk env script: $EMSDK_ENV_SCRIPT"

# shellcheck disable=SC1090
source "$EMSDK_ENV_SCRIPT"

# Force project-scoped config and cache
EM_CONFIG_FILE="$EMSDK_ROOT/.emscripten"
EM_CACHE_DIR="$EMSDK_ROOT/upstream/emscripten/cache"

export EM_CONFIG="$EM_CONFIG_FILE"
export EM_CACHE="$EM_CACHE_DIR"

TOOLCHAIN_CANDIDATES=(
  "$EMSDK_ROOT/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
  "$EMSDK_ROOT/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
)

EMSCRIPTEN_TOOLCHAIN_FILE=""
for p in "${TOOLCHAIN_CANDIDATES[@]}"; do
  if [ -f "$p" ]; then
    EMSCRIPTEN_TOOLCHAIN_FILE="$p"
    break
  fi
done

[ -n "$EMSCRIPTEN_TOOLCHAIN_FILE" ] || fail "Could not find local Emscripten.cmake"
[ -f "$EM_CONFIG_FILE" ] || fail "Could not find local emsdk config: $EM_CONFIG_FILE"

log "Running emcc sanity check"
emcc --check >/dev/null

export EM_CONFIG="$ROOT/emsdk/.emscripten"
export PATH="$ROOT/emsdk:$ROOT/emsdk/upstream/emscripten:$PATH"

log "Ensuring local vcpkg exists"
if [ ! -f "$VCPKG_ROOT/bootstrap-vcpkg.sh" ]; then
  git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT"
fi

log "Bootstrapping vcpkg"
if [ ! -x "$VCPKG_ROOT/vcpkg" ]; then
  (
    cd "$VCPKG_ROOT"
    ./bootstrap-vcpkg.sh -disableMetrics
  )
fi

[ -f "$VCPKG_MANIFEST_ROOT/vcpkg.json" ] || fail "Missing vcpkg manifest: $VCPKG_MANIFEST_ROOT/vcpkg.json"
[ -f "$VCPKG_OVERLAY_TRIPLETS/$VCPKG_TRIPLET.cmake" ] || fail "Missing overlay triplet: $VCPKG_OVERLAY_TRIPLETS/$VCPKG_TRIPLET.cmake"

log "Writing local CMake preset include"
write_local_presets "$EMSCRIPTEN_TOOLCHAIN_FILE" "$EM_CONFIG_FILE"

log "Writing local environment helper"
write_local_env "$EMSDK_ENV_SCRIPT" "$EM_CONFIG_FILE" "$EM_CACHE_DIR"

log "Installing vcpkg dependencies for $VCPKG_TRIPLET"
"$VCPKG_ROOT/vcpkg" install \
  --x-manifest-root="$VCPKG_MANIFEST_ROOT" \
  --x-install-root="$VCPKG_INSTALL_ROOT" \
  --overlay-triplets="$VCPKG_OVERLAY_TRIPLETS" \
  --triplet "$VCPKG_TRIPLET" \
  --disable-metrics

if [ -d "$ROOT/cloudflare" ]; then
  log "Installing worker dependencies"
  (
    cd "$ROOT/cloudflare"
    npm install
  )
fi

if [ -d "$ROOT/cloudflare/packages/aresWasmWorkerRuntime" ]; then
  log "Installing aresWasmWorkerRuntime dependencies"
  (
    cd "$ROOT/cloudflare/packages/aresWasmWorkerRuntime"
    npm install
    npm run build
  )
fi

log "Bootstrap complete"
printf '\n'
printf 'Local emsdk: %s\n' "$EMSDK_ROOT"
printf 'Toolchain:   %s\n' "$EMSCRIPTEN_TOOLCHAIN_FILE"
printf 'EM_CONFIG:   %s\n' "$EM_CONFIG_FILE"
printf 'Preset file: %s\n' "$LOCAL_PRESETS_FILE"
printf 'Env helper:  %s\n' "$LOCAL_ENV_FILE"
printf '\n'
printf 'Next steps:\n'
printf '  1. Include cmake-local-presets.json from CMakePresets.json\n'
printf '  2. Inherit from preset: wasm-local\n'
printf '  3. Reconfigure CLion / CMake\n'