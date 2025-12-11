@echo off
setlocal

if "%~1"=="" (
    echo Error: Please specify the build type (Debug or Release^).
    echo Usage: configure.bat [Debug^|Release]
    exit /b 1
)

set "CMAKE_BUILD_TYPE=%~1"
set "CMAKE_GENERATOR=MinGW Makefiles"
set "BUILD_DIR=output"

echo Configuring Aethel for %CMAKE_BUILD_TYPE% build...

cmake -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -S "%~dp0.." -B "%BUILD_DIR%"

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo Configuration successful. Build files are in the %BUILD_DIR% directory.
endlocal