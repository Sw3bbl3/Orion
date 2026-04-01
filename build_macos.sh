#!/bin/bash
set -e

echo "=========================================="
echo "Orion - macOS Build Script"
echo "=========================================="
echo ""

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed. Please install cmake."
    exit 1
fi

# Create Build Directory
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

echo "Configuring Project..."
cmake -G "Xcode" ..

echo ""
echo "Building Project..."
cmake --build . --config Release

echo ""
echo "=========================================="
echo "Build Successful!"
echo "App Bundle: ./build/Release/Orion.app"
echo "=========================================="
