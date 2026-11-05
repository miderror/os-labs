@echo off
echo === Pulling ====
git pull origin main

echo === Configuring CMake ===
if not exist build mkdir build
cd build
cmake -G "MinGW Makefiles" ..

echo === Compiling ===
cmake --build .

echo === Running ===
test_runner.exe
cd ..
pause