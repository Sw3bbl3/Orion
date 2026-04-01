@echo off
setlocal enabledelayedexpansion

set "DO_PACKAGE=1"
set "PACKAGE_EXTRA_ARGS="
set "SHOW_HELP="
set "INVALID_ARG="
for %%A in (%*) do call :parse_arg "%%~A"
if defined INVALID_ARG (
    echo Error: Unknown option !INVALID_ARG!
    goto :usage_fail
)
if defined SHOW_HELP goto :usage

echo ==========================================
echo Orion - Automated Build Script
echo ==========================================
echo.

:: 1. Check if CMake is installed
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: CMake is not in your PATH. Please install CMake.
    if not defined CI pause
    exit /b 1
)

:: 2. Check if we are already in a VS Developer Command Prompt
if defined VSCMD_VER (
    echo Visual Studio Environment detected.
    goto :build
)

:: 3. Try to find Visual Studio using vswhere
echo Searching for Visual Studio...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
    )
) 2>nul

if defined VS_PATH (
    echo Found Visual Studio at: "!VS_PATH!"
    if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
        echo Initializing VS Environment...
        call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
        goto :build
    )
)

:: 4. Fallback: Try standard paths
echo vswhere failed or not found. Checking standard paths...
set "PF86=%ProgramFiles(x86)%"
set "PF=%ProgramFiles%"

if exist "%PF86%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "%PF86%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto :build
)
if exist "%PF%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "%PF%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto :build
)
if exist "%PF86%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "%PF86%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto :build
)

echo.
echo Error: Could not automatically locate Visual Studio C++ Environment.
echo Please run this script from the "x64 Native Tools Command Prompt for VS".
if not defined CI pause
exit /b 1

:build
echo.
if not exist build (
    mkdir build
)

cd build

echo Configuring Project...
cmake -G "Visual Studio 17 2022" -A x64 ..
if %errorlevel% neq 0 (
    echo.
    echo Configuration Failed!
    echo Attempting to clean build cache and dependencies to fix potential corruption...

    if exist CMakeCache.txt del /f /q CMakeCache.txt
    if exist CMakeFiles rmdir /s /q CMakeFiles
    if exist _deps rmdir /s /q _deps

    echo Retrying Configuration...
    cmake -G "Visual Studio 17 2022" -A x64 ..
    if !errorlevel! neq 0 (
        echo Configuration Failed again.
        echo Retrying with default generator...
        cmake ..
        if !errorlevel! neq 0 (
            echo Configuration Failed again.
            if not defined CI pause
            exit /b 1
        )
    )
)

echo.
echo Building Project (Release)...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo.
    echo Build Failed!
    if not defined CI pause
    exit /b 1
)

if !DO_PACKAGE! EQU 1 (
    echo.
    echo Packaging Orion into distributable files...
    powershell -NoProfile -ExecutionPolicy Bypass -File "..\installer\build_release_package.ps1" -BuildDir "build" -Config "Release" -DistDir "dist" -SkipBuild !PACKAGE_EXTRA_ARGS!
    if !errorlevel! neq 0 (
        echo.
        echo Packaging Failed!
        if not defined CI pause
        exit /b 1
    )
)

echo.
echo ==========================================
echo Build Successful!
echo Executable: build\Release\Orion.exe
if !DO_PACKAGE! EQU 1 (
echo Package Folder: dist\Orion
echo Installer Output: dist\installer
)
echo ==========================================
echo.
exit /b 0

:parse_arg
if /I "%~1"=="--package" (
    set "DO_PACKAGE=1"
    goto :eof
)
if /I "%~1"=="--build-only" (
    set "DO_PACKAGE=0"
    goto :eof
)
if /I "%~1"=="--unsigned" (
    set "PACKAGE_EXTRA_ARGS=!PACKAGE_EXTRA_ARGS! -SkipCodeSign"
    goto :eof
)
if /I "%~1"=="--skip-installer" (
    set "PACKAGE_EXTRA_ARGS=!PACKAGE_EXTRA_ARGS! -SkipInstaller"
    goto :eof
)
if /I "%~1"=="--help" (
    set "SHOW_HELP=1"
    goto :eof
)
set "INVALID_ARG=%~1"
goto :eof

:usage
echo Usage: build_windows.bat [--package] [--build-only] [--unsigned] [--skip-installer]
echo.
echo   --package         Build and package Orion (default).
echo   --build-only      Build Orion only, no packaging.
echo   --unsigned        Package without code signing.
echo   --skip-installer  Package app and ZIP, but skip Inno Setup installer build.
exit /b 0

:usage_fail
echo.
echo Run build_windows.bat --help for usage.
exit /b 1
