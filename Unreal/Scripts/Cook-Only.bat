@echo off
REM ============================================================
REM  Cook content only (no build/stage/package)
REM  Useful to verify all assets cook without errors.
REM ============================================================

call "%~dp0_FindUE.bat" || exit /b 1
set PROJECT_PATH=%~dp0..\CubeTacToe.uproject
set PLATFORM=Win64

echo ========================================
echo  Cooking content for %PLATFORM%
echo ========================================

call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
    -project="%PROJECT_PATH%" ^
    -platform=%PLATFORM% ^
    -clientconfig=Shipping ^
    -cook ^
    -skipstage ^
    -utf8output ^
    -nocompile

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Cook failed with exit code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo  Cook complete!
pause
