#!/usr/bin/env bash
set -euo pipefail

# Modern test runner using CMake
# Usage: ./test.sh [cmake-args] [-- [test-args]]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build"

# Extract test arguments if present
TEST_ARGS=""
CMAKE_ARGS=""
found_separator=false

for arg in "$@"; do
    if [[ "$arg" == "--" ]]; then
        found_separator=true
        continue
    fi
    
    if $found_separator; then
        TEST_ARGS="$TEST_ARGS $arg"
    else
        CMAKE_ARGS="$CMAKE_ARGS $arg"
    fi
done

echo "[test] Setting up build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "[test] Configuring with CMake..."
cmake .. $CMAKE_ARGS

echo "[test] Building tests..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "[test] Running tests..."
./native_tests $TEST_ARGS
