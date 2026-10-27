@echo off
REM Builds with the build directory outside Documents. Windows Defender's Controlled Folder
REM Access can block the toolchain's ld.exe / as.exe from writing inside protected folders
REM (see WINDOWS_BUILD.md). Extra arguments are passed on, e.g. BUILD_WINDOWS.bat -t upload

set PLATFORMIO_BUILD_DIR=C:\tmp\pio_build
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" run -e supermini_esp32c3 %*
if errorlevel 1 (
    echo Build failed
    exit /b 1
)
echo Firmware: %PLATFORMIO_BUILD_DIR%\supermini_esp32c3\firmware.bin
