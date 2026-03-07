@echo off
REM ============================================================
REM  Shared packaging logic for CubeTacToe (Win64)
REM
REM  Usage:  call _Package.bat <Config>
REM    e.g.: call _Package.bat Shipping
REM           call _Package.bat Development
REM ============================================================

set CONFIG=%~1

if /i "%CONFIG%"=="Shipping" (
    set OUTPUT_DIR=%~dp0..\PackagedBuilds\Windows
    set ZIP_NAME=CubeTacToe
    set EXTRA_FLAGS=-nodebuginfo
) else if /i "%CONFIG%"=="Development" (
    set OUTPUT_DIR=%~dp0..\PackagedBuilds\Windows-Dev
    set ZIP_NAME=CubeTacToe-Development
    set EXTRA_FLAGS=
) else (
    echo [ERROR] Usage: _Package.bat ^<Shipping^|Development^>
    exit /b 1
)

call "%~dp0_FindUE.bat" || exit /b 1
set PROJECT_PATH=%~dp0..\CubeTacToe.uproject
set PLATFORM=Win64

echo ========================================
echo  Packaging CubeTacToe (%CONFIG% / %PLATFORM%)
echo ========================================

if exist "%OUTPUT_DIR%" (
    echo Removing existing output directory: %OUTPUT_DIR%
    rmdir /s /q "%OUTPUT_DIR%"
    REM also remove zip file if it exists
    if exist "%~dp0..\PackagedBuilds\%ZIP_NAME%.zip" (
        echo Removing existing zip file: %~dp0..\PackagedBuilds\%ZIP_NAME%.zip
        del "%~dp0..\PackagedBuilds\%ZIP_NAME%.zip"
    )
)

call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
    -project="%PROJECT_PATH%" ^
    -platform=%PLATFORM% ^
    -clientconfig=%CONFIG% ^
    -cook ^
    -stage ^
    -package ^
    -archive ^
    -archivedirectory="%OUTPUT_DIR%" ^
    -pak ^
    -prereqs ^
    -utf8output ^
    -build ^
    %EXTRA_FLAGS%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Packaging failed with exit code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

set ZIP_FILE=%~dp0..\PackagedBuilds\%ZIP_NAME%.zip
if exist "%ZIP_FILE%" del "%ZIP_FILE%"
echo Creating zip: %ZIP_FILE%
powershell -NoProfile -Command "Compress-Archive -Path '%OUTPUT_DIR%\*' -DestinationPath '%ZIP_FILE%'"

echo.
echo ========================================
echo  Build complete!
echo  Output: %OUTPUT_DIR%
echo  Zip:    %ZIP_FILE%
echo ========================================
pause
