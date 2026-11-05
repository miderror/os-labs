#!/usr/bin/env bash
set -e

echo "=== Pulling ===="
git pull origin main || true

echo "=== Configuring CMake ==="
mkdir -p build
cd build
cmake ..

echo "=== Compiling ==="
cmake --build .

echo "=== Running ==="
./test_runner