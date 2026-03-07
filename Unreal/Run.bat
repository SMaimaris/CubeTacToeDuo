@echo off
REM ============================================================
REM  Run CubeTacToe packaged build
REM  Usage:  Run.bat         (Shipping, default)
REM          Run.bat -dev    (Development)
REM ============================================================

if /i "%~1"=="-dev" (
    set BUILD_DIR=%~dp0PackagedBuilds\Windows-Dev
) else (
    set BUILD_DIR=%~dp0PackagedBuilds\Windows
)

set EXE=%BUILD_DIR%\CubeTacToe.exe

if not exist "%EXE%" (
    echo [ERROR] Build not found: %EXE%
    echo         Run a packaging script first.
    pause
    exit /b 1
)

start "" "%EXE%"
