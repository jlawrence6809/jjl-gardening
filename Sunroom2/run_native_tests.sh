#!/usr/bin/env bash
set -euo pipefail

# Run platform-neutral rule_core tests locally (no hardware, no PIO test runner).
# - Ensures ArduinoJson headers are present in .pio/libdeps/native
# - Builds src/rule_core.cpp + src/rule_core_runner.cpp with clang++/g++
# - Executes the resulting binary
# - Added because we couldn't get native pio tests to work

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

INC_DIR=".pio/libdeps/native/ArduinoJson/src"
GTEST_CELLAR_PREFIX="/opt/homebrew/Cellar/googletest/1.17.0"
GTEST_INC_DIR="$GTEST_CELLAR_PREFIX/include"
GTEST_LIB_DIR="$GTEST_CELLAR_PREFIX/lib"
BUILD_DIR=".native_build"
BIN="$BUILD_DIR/rule_core_runner"

if [[ ! -d "$INC_DIR" ]]; then
  echo "[native-tests] Installing ArduinoJson headers via PlatformIO (one-time)..."
  # We only need headers in .pio/libdeps/native; allow build to fail if linking has no inputs
  pio run -e native || true
fi

if [[ ! -d "$INC_DIR" ]]; then
  echo "[native-tests] Error: ArduinoJson headers not found at $INC_DIR"
  echo "Try: pio run -e native"
  exit 1
fi

mkdir -p "$BUILD_DIR"

CXX_BIN="${CXX:-}"
if [[ -z "${CXX_BIN}" ]]; then
  if command -v clang++ >/dev/null 2>&1; then CXX_BIN="clang++"; else CXX_BIN="g++"; fi
fi

echo "[native-tests] Building with $CXX_BIN..."
"$CXX_BIN" -std=c++17 -O2 \
  -I"$INC_DIR" -I"$GTEST_INC_DIR" \
  src/automation_dsl/core.cpp \
  src/automation_dsl/registry_functions.cpp \
  src/automation_dsl/bridge_functions.cpp \
  src/automation_dsl/time_helpers.cpp \
  tests_native/test_mocks.cpp \
  tests_native/unified_value_tests.cpp \
  tests_native/function_registry_tests.cpp \
  tests_native/bridge_integration_tests.cpp \
  -L"$GTEST_LIB_DIR" -lgtest -lgtest_main -lpthread \
  -o "$BIN"

echo "[native-tests] Running $BIN $*"
"$BIN" "$@"


