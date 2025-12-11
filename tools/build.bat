@echo off
setlocal

set "BUILD_DIR=output"
set "EXECUTABLE_NAME=Aethel.exe"

echo Building Aethel...
cmake --build "%BUILD_DIR%"

if %errorlevel% neq 0 (
    echo BUILD FAILED! Check error messages above.
    exit /b 1
)

echo Build successful.

echo Running %EXECUTABLE_NAME%...
cd "%BUILD_DIR%\bin"
"%EXECUTABLE_NAME%"

endlocal