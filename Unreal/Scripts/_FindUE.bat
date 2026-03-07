@echo off
REM ============================================================
REM  Resolves UE_ROOT from environment variable.
REM  Set UE_ROOT in your system environment to your UE install,
REM  e.g.:  UE_ROOT=C:\Program Files\Epic Games\UE_5.7
REM
REM  Called by the other build scripts via: call _FindUE.bat
REM ============================================================

if defined UE_ROOT (
    if exist "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" (
        echo [OK] UE_ROOT = %UE_ROOT%
        goto :eof
    )
    echo [ERROR] UE_ROOT is set to "%UE_ROOT%" but RunUAT.bat was not found there.
    echo         Please verify the path points to a valid Unreal Engine installation.
    pause
    exit /b 1
)

echo [ERROR] UE_ROOT environment variable is not set.
echo.
echo  Please create a system environment variable called UE_ROOT
echo  pointing to your Unreal Engine installation root, e.g.:
echo.
echo    setx UE_ROOT "C:\Program Files\Epic Games\UE_5.7"
echo.
echo  Then restart your terminal and run this script again.
pause
exit /b 1
