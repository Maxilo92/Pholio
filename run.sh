#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
CONFIG="Release"

resolve_binary() {
    if [[ "$(uname -s)" != "Darwin" ]]; then
        echo "$BUILD_DIR/Pholio"
        return 0
    fi

    local best_binary
    best_binary="$(python3 - "$PROJECT_ROOT" "$BUILD_DIR" <<'PY'
import os
import sys
import plistlib
from pathlib import Path

project_root = Path(sys.argv[1])
build_dir = Path(sys.argv[2])

def parse_version(v: str):
    import re
    m = re.match(r"^\s*(\d+)\.(\d+)\.(\d+)(?:-([A-Za-z0-9.\-]+))?\s*$", v or "")
    if not m:
        return None
    major, minor, patch = int(m.group(1)), int(m.group(2)), int(m.group(3))
    suffix = m.group(4) or ""
    if suffix == "":
        suffix_rank = 3
    elif suffix.startswith("rc"):
        suffix_rank = 2
    elif suffix.startswith("beta"):
        suffix_rank = 1
    else:
        suffix_rank = 0
    return (major, minor, patch, suffix_rank, suffix)

def app_candidates():
    seen = set()
    preferred_roots = (
        build_dir,
        project_root / "build-release",
    )

    for base in preferred_roots:
        if not base.exists():
            continue
        for p in base.rglob("Pholio*.app"):
            if p.is_dir():
                resolved = p.resolve()
                if resolved not in seen:
                    seen.add(resolved)
                    yield resolved

    # Fallback: search in project, but ignore package export folders.
    if project_root.exists():
        for p in project_root.rglob("Pholio*.app"):
            if "_CPack_Packages" in p.parts:
                continue
            if p.is_dir():
                resolved = p.resolve()
                if resolved not in seen:
                    seen.add(resolved)
                    yield resolved

best = None
best_key = None
for app in app_candidates():
    info = app / "Contents" / "Info.plist"
    binary = app / "Contents" / "MacOS" / "Pholio"
    if not binary.is_file():
        continue
    version = "0.0.0"
    if info.is_file():
        try:
            with info.open("rb") as f:
                plist = plistlib.load(f)
            version = str(plist.get("CFBundleShortVersionString") or plist.get("CFBundleVersion") or "0.0.0")
        except Exception:
            pass
    parsed = parse_version(version) or (0, 0, 0, -1, "")
    mtime = binary.stat().st_mtime
    key = (parsed, mtime)
    if best is None or key > best_key:
        best = binary
        best_key = key

if best is not None:
    print(str(best))
PY
)"

    if [[ -n "$best_binary" ]]; then
        echo "$best_binary"
    else
        echo "$BUILD_DIR/Pholio.app/Contents/MacOS/Pholio"
    fi
}

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

repair_vcpkg_repo() {
    local vcpkg_root="$1"
    local configured_worktree=""
    configured_worktree="$(git -C "$vcpkg_root" config --get core.worktree 2>/dev/null || true)"

    if [[ "$configured_worktree" == ".git" || "$configured_worktree" == "$vcpkg_root/.git" ]]; then
        echo "Detected broken vcpkg git worktree: $configured_worktree"
        echo "Repairing vcpkg git config (core.worktree)..."
        git -C "$vcpkg_root" config --unset core.worktree 2>/dev/null || true
    fi

    if git -C "$vcpkg_root" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        echo "Updating local vcpkg refs..."
        git -C "$vcpkg_root" fetch --all --tags --prune >/dev/null 2>&1 || true
    fi
}

build_app() {
    local vcpkg_root
    if ! vcpkg_root="$(find_vcpkg)"; then
        echo "Error: vcpkg not found. Set VCPKG_ROOT or install at ~/vcpkg."
        return 1
    fi
    repair_vcpkg_repo "$vcpkg_root"

    local toolchain_file="$vcpkg_root/scripts/buildsystems/vcpkg.cmake"
    local cmake_generator="Unix Makefiles"
    local -a cmake_configure=(
        cmake
        -B "$BUILD_DIR"
        -S "$PROJECT_ROOT"
        "-DCMAKE_BUILD_TYPE=$CONFIG"
        "-DCMAKE_TOOLCHAIN_FILE=$toolchain_file"
    )

    if command -v ninja >/dev/null 2>&1; then
        cmake_generator="Ninja"
        cmake_configure+=(-G Ninja)
    fi

    if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
        local existing_generator
        existing_generator="$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "$BUILD_DIR/CMakeCache.txt" | head -n 1)"
        if [[ -n "$existing_generator" && "$existing_generator" != "$cmake_generator" ]]; then
            echo "Detected CMake generator mismatch ($existing_generator vs $cmake_generator). Resetting build cache..."
            rm -f "$BUILD_DIR/CMakeCache.txt"
            rm -rf "$BUILD_DIR/CMakeFiles"
        fi
    fi

    if [[ "$PROJECT_ROOT" == *" "* ]]; then
        cmake_configure+=("-DVCPKG_INSTALLED_DIR=/tmp/vcpkg_installed_pholio")
    fi

    echo "Configuring project..."
    if ! "${cmake_configure[@]}"; then
        return 1
    fi
    echo "Building project..."
    if ! cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel; then
        return 1
    fi
}

read_expected_version() {
    local version_file="$PROJECT_ROOT/VERSION"
    if [[ -f "$version_file" ]]; then
        tr -d '[:space:]' < "$version_file"
    fi
}

read_binary_version() {
    local binary_path="$1"
    if [[ "$(uname -s)" != "Darwin" ]]; then
        return 0
    fi

    python3 - "$binary_path" <<'PY'
import sys
import plistlib
from pathlib import Path

binary = Path(sys.argv[1]).resolve()
parts = list(binary.parts)
try:
    idx = parts.index("Contents")
except ValueError:
    print("")
    raise SystemExit(0)

app_root = Path(*parts[:idx])
info = app_root / "Contents" / "Info.plist"
if not info.is_file():
    print("")
    raise SystemExit(0)

try:
    with info.open("rb") as f:
        plist = plistlib.load(f)
    version = str(plist.get("CFBundleShortVersionString") or plist.get("CFBundleVersion") or "")
    print(version.strip())
except Exception:
    print("")
PY
}

ensure_expected_version() {
    local expected_version
    expected_version="$(read_expected_version)"
    if [[ -z "$expected_version" ]]; then
        return 0
    fi

    local resolved_binary
    resolved_binary="$(resolve_binary)"
    if [[ ! -f "$resolved_binary" ]]; then
        return 0
    fi

    local actual_version
    actual_version="$(read_binary_version "$resolved_binary")"
    if [[ -n "$actual_version" && "$actual_version" != "$expected_version" ]]; then
        echo "Detected local app version $actual_version, expected $expected_version from VERSION."
        echo "Attempting rebuild to sync local binary version..."
        if ! build_app; then
            echo "Error: Rebuild failed; refusing to launch outdated version $actual_version."
            echo "Fix vcpkg first (e.g. git -C \"\$HOME/vcpkg\" fetch --all --tags --prune) and rerun."
            return 1
        fi

        resolved_binary="$(resolve_binary)"
        actual_version="$(read_binary_version "$resolved_binary")"
        if [[ -n "$actual_version" && "$actual_version" != "$expected_version" ]]; then
            echo "Error: Local binary is still version $actual_version after rebuild, expected $expected_version."
            return 1
        fi
    fi

    return 0
}

if [ ! -f "$(resolve_binary)" ]; then
    echo "Executable not found. Building first..."
    build_app
fi

if [ ! -f "$(resolve_binary)" ]; then
    echo "Error: Executable not found even after build."
    exit 1
fi

if ! ensure_expected_version; then
    exit 1
fi

while true; do
    BINARY="$(resolve_binary)"
    if [ ! -f "$BINARY" ]; then
        echo "No runnable local Pholio binary found."
        exit 1
    fi

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
        if ! ensure_expected_version; then
            exit 1
        fi
        BINARY="$(resolve_binary)"
        if [ ! -f "$BINARY" ]; then
            echo "Build completed but executable is still missing."
            exit 1
        fi
    else
        echo "Application exited normally (Exit Code $EXIT_CODE)."
        exit $EXIT_CODE
    fi
    
    echo "----------------------------------------"
    sleep 1
done
