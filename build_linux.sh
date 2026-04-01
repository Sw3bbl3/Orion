#!/bin/bash
set -e

echo "=========================================="
echo "Orion - Linux Build Script"
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
# Default to Ninja if available, else Makefiles
if command -v ninja &> /dev/null; then
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
else
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

echo ""
echo "Building Project..."
cmake --build . --config Release --parallel $(nproc)

echo ""
echo "=========================================="
echo "Build Successful!"
echo "Executable: ./build/Orion"
echo "=========================================="
