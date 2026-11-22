@echo off
setlocal
set "ROOT=%~dp0"
set "BIN=%ROOT%build\windows\flatcore.exe"

if not exist "%BIN%" (
    echo error: build Flatcore first with build-windows.bat
    exit /b 1
)

"%BIN%" help >nul
if errorlevel 1 goto failed

"%BIN%" true
if errorlevel 1 goto failed

"%BIN%" false
if not errorlevel 1 goto failed

for /f "delims=" %%A in ('"%BIN%" echo Flatcore-Windows-smoke') do set "ECHO_RESULT=%%A"
if not "%ECHO_RESULT%"=="Flatcore-Windows-smoke" goto failed

for /f "delims=" %%A in ('"%BIN%" pwd') do set "PWD_RESULT=%%A"
if not defined PWD_RESULT goto failed

for /f "delims=" %%A in ('"%BIN%" hostname') do set "HOST_RESULT=%%A"
if not defined HOST_RESULT goto failed

echo Flatcore Windows smoke tests passed.
exit /b 0

:failed
echo Flatcore Windows smoke test failed.
exit /b 1
