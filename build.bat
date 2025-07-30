@echo off
setlocal

echo =============================
echo Building the project...
echo =============================

REM Force 64-bit MSVC environment
call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
    echo [ERROR] Failed to initialize Visual Studio environment.
    pause
    exit /b 1
)


REM Configure with CMake
if not exist build (
    mkdir build
)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

REM Build the project
cmake --build build
if errorlevel 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo =============================
echo Build complete! Output is in build/
echo =============================

pause