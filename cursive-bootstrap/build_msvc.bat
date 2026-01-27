@echo off
REM Build Cursive compiler using MSVC Build Tools
REM
REM Prerequisites:
REM   - Run setup_third_party.ps1 first to download dependencies
REM   - Visual Studio Build Tools installed
REM
REM Usage:
REM   build_msvc.bat [Release|Debug]

setlocal

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set SCRIPT_DIR=%~dp0
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

set BUILD_DIR=%SCRIPT_DIR%\build
set CMAKE_EXE=%SCRIPT_DIR%\third_party\cmake\bin\cmake.exe
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if not exist %VCVARS% (
    echo ERROR: VS Build Tools not found at %VCVARS%
    exit /b 1
)

if not exist "%CMAKE_EXE%" (
    echo ERROR: CMake not found at %CMAKE_EXE%
    echo Please run: powershell -ExecutionPolicy Bypass -File tools\setup_third_party.ps1
    exit /b 1
)

echo.
echo ========================================
echo   Cursive Compiler - MSVC Build
echo   Build Type: %BUILD_TYPE%
echo ========================================
echo.

REM Set up MSVC environment
echo Setting up MSVC environment...
call %VCVARS%
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to set up MSVC environment
    exit /b 1
)

echo.
echo Configuring with CMake...
"%CMAKE_EXE%" -B "%BUILD_DIR%" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_TESTING=OFF -S "%SCRIPT_DIR%"
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    exit /b 1
)

echo.
echo Building...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --config %BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo ========================================
echo   Build completed successfully!
echo   Output: %BUILD_DIR%\cursivec0.exe
echo ========================================
echo.
