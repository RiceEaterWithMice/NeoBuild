@echo off
rem Sets up the MSVC developer environment (x64 or x86) and runs a CMake
rem preset task. Usage: devcmd.cmd x64|x86 <preset> [configure|build|clean]

set "ARCH=%~1"
set "PRESET=%~2"
set "ACTION=%~3"
if "%ACTION%"=="" set "ACTION=build"

if /i "%ARCH%"=="x86" (set "VCARCH=x86") else set "VCARCH=x64"

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"

if not defined VSROOT (
    echo Visual Studio C++ tools not found. Install Visual Studio with the Desktop C++ workload.
    exit /b 1
)

call "%VSROOT%\VC\Auxiliary\Build\vcvarsall.bat" %VCARCH%
if errorlevel 1 exit /b 1

if /i "%ACTION%"=="configure" (
    cmake --preset %PRESET%
) else if /i "%ACTION%"=="clean" (
    cmake --build --preset %PRESET% --target clean
) else (
    cmake --preset %PRESET% && cmake --build --preset %PRESET%
)
