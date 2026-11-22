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

rem FASM searches INCLUDE for win64a.inc.
set "FASM_PATH="
for /f "delims=" %%I in ('where "%FASM%" 2^>nul') do if not defined FASM_PATH set "FASM_PATH=%%I"
for %%I in ("%FASM_PATH%") do set "FASM_DIR=%%~dpI"
if exist "%FASM_DIR%include\win64a.inc" set "INCLUDE=%FASM_DIR%include;%INCLUDE%"

if not exist "%ROOT%build\windows" mkdir "%ROOT%build\windows"
"%FASM%" "%ROOT%src\platform\windows\flatcore.asm" "%ROOT%build\windows\flatcore.exe"
if errorlevel 1 exit /b %errorlevel%

echo Built: %ROOT%build\windows\flatcore.exe
if /i "%~1"=="test" (
    call "%ROOT%tests\windows-smoke.bat"
    exit /b %errorlevel%
)
exit /b 0
