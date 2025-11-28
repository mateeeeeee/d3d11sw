#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SHADER_DIR="$SCRIPT_DIR"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
FXC2_DIR="$REPO_ROOT/third_party/fxc2"
FXC2_EXE="$FXC2_DIR/fxc2.exe"

if [ ! -f "$FXC2_EXE" ]; then
    echo "Error: fxc2.exe not found at $FXC2_EXE"
    echo "Build it once:  brew install mingw-w64 && make -C third_party/fxc2 x86"
    exit 1
fi

OS="$(uname -s)"
case "$OS" in
    MINGW*|MSYS*|CYGWIN*|Windows_NT)
        FXC2="$FXC2_EXE"
        ;;
    *)
        if ! command -v wine &>/dev/null; then
            echo "Error: wine is required on $OS to run fxc2.exe"
            echo "  macOS:  brew install --cask wine-stable"
            echo "  Linux:  apt install wine  (or equivalent)"
            exit 1
        fi
        FXC2="wine $FXC2_EXE"
        ;;
esac

MISSING_ONLY=false
if [[ "${1:-}" == "--missing" ]]; then
    MISSING_ONLY=true
fi

compile_shader() {
    local hlsl="$1"
    local out="${hlsl%.hlsl}.o"
    local name="$(basename "$hlsl")"

    if $MISSING_ONLY && [ -f "$out" ]; then
        return
    fi

    if [ -f "$out" ] && [ "$out" -nt "$hlsl" ]; then
        return
    fi

    $FXC2 -T cs_5_0 -E main -Fo"$out" "$hlsl"
    echo "  $name -> $(basename "$out")"
}

echo "Compiling golden shaders..."
for hlsl in "$SHADER_DIR"/*.hlsl; do
    [ -f "$hlsl" ] || continue
    compile_shader "$hlsl"
done
echo "Done."
