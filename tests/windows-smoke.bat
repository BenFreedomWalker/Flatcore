@echo off
setlocal
set "ROOT=%~dp0.."
set "BIN=%ROOT%\build\windows\flatcore.exe"

if not exist "%BIN%" (
    echo error: build Flatcore first with build-windows.bat
    exit /b 1
)

set "STEP=help dispatch"
"%BIN%" help >nul
if errorlevel 1 goto failed

set "STEP=true exit status"
"%BIN%" true
if errorlevel 1 goto failed

set "STEP=false exit status"
"%BIN%" false
if not errorlevel 1 goto failed

set "ECHO_RESULT="
set "STEP=echo output"
for /f "delims=" %%A in ('"%BIN%" echo Flatcore-Windows-smoke') do set "ECHO_RESULT=%%A"
if not "%ECHO_RESULT%"=="Flatcore-Windows-smoke" goto failed

set "PWD_RESULT="
set "STEP=pwd output"
for /f "delims=" %%A in ('"%BIN%" pwd') do set "PWD_RESULT=%%A"
if not defined PWD_RESULT goto failed

set "HOST_RESULT="
set "STEP=hostname output"
for /f "delims=" %%A in ('"%BIN%" hostname') do set "HOST_RESULT=%%A"
if not defined HOST_RESULT goto failed

set "TEST_DIR=%TEMP%\flatcore-smoke-%RANDOM%-%RANDOM%"
set "STEP=mkdir path with spaces"
"%BIN%" mkdir "%TEST_DIR%"
if errorlevel 1 goto failed

> "%TEST_DIR%\sample.txt" echo alpha
set "HEAD_RESULT="
for /f "delims=" %%A in ('"%BIN%" head "%TEST_DIR%\sample.txt"') do set "HEAD_RESULT=%%A"
if not "%HEAD_RESULT%"=="alpha" goto failed

set "STEP=ls directory"
"%BIN%" ls "%TEST_DIR%" >nul
if errorlevel 1 goto failed
"%BIN%" cp "%TEST_DIR%\sample.txt" "%TEST_DIR%\copy.txt"
if errorlevel 1 goto failed
"%BIN%" mv "%TEST_DIR%\copy.txt" "%TEST_DIR%\moved.txt"
if errorlevel 1 goto failed
"%BIN%" cat "%TEST_DIR%\moved.txt" >nul
if errorlevel 1 goto failed

set "BASE_RESULT="
for /f "delims=" %%A in ('"%BIN%" basename "%TEST_DIR%\sample.txt"') do set "BASE_RESULT=%%A"
if not "%BASE_RESULT%"=="sample.txt" goto failed
set "DIR_RESULT="
for /f "delims=" %%A in ('"%BIN%" dirname "%TEST_DIR%\sample.txt"') do set "DIR_RESULT=%%A"
if not "%DIR_RESULT%"=="%TEST_DIR%" goto failed

"%BIN%" rm "%TEST_DIR%\sample.txt"
if errorlevel 1 goto failed
"%BIN%" rm "%TEST_DIR%\moved.txt"
if errorlevel 1 goto failed
set "STEP=rmdir test directory"
"%BIN%" rmdir "%TEST_DIR%"
if errorlevel 1 goto failed

echo Flatcore Windows smoke tests passed.
exit /b 0

 :failed
echo Flatcore Windows smoke test failed at: %STEP%
exit /b 1
