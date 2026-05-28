#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "========================================"
echo "  Stacktrace Build & Test Script"
echo "========================================"
echo ""

# 平台检测
PLATFORM=$(uname -s)
ARCH=$(uname -m)
echo "[INFO] Platform: $PLATFORM, Architecture: $ARCH"

if [ "$PLATFORM" = "Darwin" ]; then
    echo "[INFO] Detected macOS platform"
elif [ "$PLATFORM" = "Linux" ]; then
    echo "[INFO] Detected Linux platform"
else
    echo "[WARN] Unknown platform: $PLATFORM"
fi
echo ""

# Clean build directory
echo "[1/4] Cleaning build directory..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Configure with cmake
echo "[2/4] Configuring project..."
cd "$BUILD_DIR"
cmake ..

# Build
echo "[3/4] Building project..."
cmake --build . --parallel

# Run tests
echo "[4/4] Running unit tests..."
ctest --output-on-failure

echo ""
echo "========================================"
echo "  Build completed successfully!"
echo "========================================"
echo ""
echo "Run the application:"
echo "  $BUILD_DIR/hello"
echo ""
echo "Run a specific test case:"
echo "  $BUILD_DIR/hello <test_number>"
echo "  e.g., $BUILD_DIR/hello 24  # bad_cast test"