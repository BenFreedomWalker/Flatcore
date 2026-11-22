@echo off
setlocal

set "ROOT=%~dp0"
set "FASM=%FASM%"
if not defined FASM set "FASM=fasm"

where "%FASM%" >nul 2>nul
if errorlevel 1 (
    echo error: FASM was not found. Add fasm.exe to PATH or set FASM.
    exit /b 127
)

if not exist "%ROOT%build\windows" mkdir "%ROOT%build\windows"
"%FASM%" "%ROOT%src\platform\windows\flatcore.asm" "%ROOT%build\windows\flatcore.exe"
if errorlevel 1 exit /b %errorlevel%

echo Built: %ROOT%build\windows\flatcore.exe
if /i "%~1"=="test" call "%ROOT%tests\windows-smoke.bat"
exit /b %errorlevel%
