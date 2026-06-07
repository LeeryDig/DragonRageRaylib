@echo off
setlocal

set ROOT_DIR=%~dp0..
set BUILD_DIR=%ROOT_DIR%\build\windows-debug
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat

if exist "%VCVARS%" call "%VCVARS%"

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Debug
if errorlevel 1 exit /b 1

echo OK: %BUILD_DIR%\bin\DragonRage.exe
