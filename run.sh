#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
CONFIG="Release"

if [[ "$(uname -s)" == "Darwin" ]]; then
    APP_BUNDLE="$BUILD_DIR/Pholio.app"
    BINARY="$APP_BUNDLE/Contents/MacOS/Pholio"
else
    BINARY="$BUILD_DIR/Pholio"
fi

find_vcpkg() {
    local candidates=(
        "${VCPKG_ROOT:-}"
        "$HOME/vcpkg"
        "/usr/local/opt/vcpkg"
        "/opt/vcpkg"
    )
    for root in "${candidates[@]}"; do
        if [ -n "$root" ] && [ -f "$root/scripts/buildsystems/vcpkg.cmake" ]; then
            echo "$root"
            return 0
        fi
    done
    return 1
}

build_app() {
    local vcpkg_root
    if ! vcpkg_root="$(find_vcpkg)"; then
        echo "Error: vcpkg not found. Set VCPKG_ROOT or install at ~/vcpkg."
        return 1
    fi

    local toolchain_file="$vcpkg_root/scripts/buildsystems/vcpkg.cmake"
    local -a cmake_configure=(
        cmake
        -B "$BUILD_DIR"
        -S "$PROJECT_ROOT"
        "-DCMAKE_BUILD_TYPE=$CONFIG"
        "-DCMAKE_TOOLCHAIN_FILE=$toolchain_file"
    )

    if command -v ninja >/dev/null 2>&1; then
        cmake_configure+=(-G Ninja)
    fi

    if [[ "$PROJECT_ROOT" == *" "* ]]; then
        cmake_configure+=("-DVCPKG_INSTALLED_DIR=/tmp/vcpkg_installed_pholio")
    fi

    echo "Configuring project..."
    "${cmake_configure[@]}"
    echo "Building project..."
    cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel
}

if [ ! -f "$BINARY" ]; then
    echo "Executable not found. Building first..."
    build_app
fi

if [ ! -f "$BINARY" ]; then
    echo "Error: Executable not found at $BINARY even after build."
    exit 1
fi

while true; do
    echo "Starting Pholio..."
    export PHOLIO_RESTART_VIA_EXIT_CODE=1
    "$BINARY" "$@"
    EXIT_CODE=$?
    
    if [ $EXIT_CODE -eq 42 ]; then
        echo "Restart requested by application (Exit Code 42)."
        # Just continue the loop
    elif [ $EXIT_CODE -eq 43 ]; then
        echo "Rebuild and restart requested (Exit Code 43). Rebuilding..."
        build_app
        if [ ! -f "$BINARY" ]; then
            echo "Build completed but executable is still missing: $BINARY"
            exit 1
        fi
    else
        echo "Application exited normally (Exit Code $EXIT_CODE)."
        exit $EXIT_CODE
    fi
    
    echo "----------------------------------------"
    sleep 1
done
