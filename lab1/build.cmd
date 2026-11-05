@echo off
echo === Pulling ====
git pull origin main

echo === Configuring CMake ===
if not exist build mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..

echo === Compiling ===
cmake --build .

echo === Running ===
hello_app.exe
cd ..
pause