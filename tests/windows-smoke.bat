@echo off
setlocal
set "ROOT=%~dp0.."
set "BIN=%ROOT%\build\windows\flatcore.exe"
set "OUT=%TEMP%\flatcore-output-%RANDOM%-%RANDOM%.txt"
set "EXPECTED=%TEMP%\flatcore-expected-%RANDOM%-%RANDOM%.txt"

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

set "STEP=echo output"
"%BIN%" echo Flatcore-Windows-smoke > "%OUT%"
set "ECHO_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Flatcore-Windows-smoke" set "ECHO_MATCH=1"
if not defined ECHO_MATCH goto failed

set "STEP=pwd output"
"%BIN%" pwd > "%OUT%"
if errorlevel 1 goto failed
for %%A in ("%OUT%") do if "%%~zA"=="0" goto failed

set "STEP=hostname output"
"%BIN%" hostname > "%OUT%"
if errorlevel 1 goto failed
for %%A in ("%OUT%") do if "%%~zA"=="0" goto failed

set "TEST_DIR=%TEMP%\flatcore-smoke-%RANDOM%-%RANDOM%"
set "STEP=mkdir path with spaces"
"%BIN%" mkdir "%TEST_DIR%"
if errorlevel 1 goto failed

set "STEP=create sample file"
> "%TEST_DIR%\sample.txt" echo alpha
if not exist "%TEST_DIR%\sample.txt" goto failed

set "STEP=head output"
"%BIN%" head "%TEST_DIR%\sample.txt" > "%OUT%"
if errorlevel 1 goto failed
> "%EXPECTED%" echo alpha
fc /b "%OUT%" "%EXPECTED%" >nul
if errorlevel 1 goto failed

set "STEP=ls directory"
"%BIN%" ls "%TEST_DIR%" >nul
if errorlevel 1 goto failed

set "STEP=copy file"
"%BIN%" cp "%TEST_DIR%\sample.txt" "%TEST_DIR%\copy.txt"
if errorlevel 1 goto failed

set "STEP=move file"
"%BIN%" mv "%TEST_DIR%\copy.txt" "%TEST_DIR%\moved.txt"
if errorlevel 1 goto failed

set "STEP=cat file"
"%BIN%" cat "%TEST_DIR%\moved.txt" >nul
if errorlevel 1 goto failed

set "STEP=basename output"
"%BIN%" basename "%TEST_DIR%\sample.txt" > "%OUT%"
set "BASENAME_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="sample.txt" set "BASENAME_MATCH=1"
if not defined BASENAME_MATCH goto failed

set "STEP=basename drive root"
"%BIN%" basename "C:\" > "%OUT%"
set "BASENAME_ROOT_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="\" set "BASENAME_ROOT_MATCH=1"
if not defined BASENAME_ROOT_MATCH goto failed

set "STEP=dirname output"
"%BIN%" dirname "%TEST_DIR%\sample.txt" > "%OUT%"
set "DIRNAME_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="%TEST_DIR%" set "DIRNAME_MATCH=1"
if not defined DIRNAME_MATCH goto failed

set "STEP=dirname drive root"
"%BIN%" dirname "C:\file.txt" > "%OUT%"
set "DIRNAME_ROOT_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="C:\" set "DIRNAME_ROOT_MATCH=1"
if not defined DIRNAME_ROOT_MATCH goto failed

set "STEP=remove sample file"
"%BIN%" rm "%TEST_DIR%\sample.txt"
if errorlevel 1 goto failed

set "STEP=remove moved file"
"%BIN%" rm "%TEST_DIR%\moved.txt"
if errorlevel 1 goto failed

set "STEP=rmdir test directory"
"%BIN%" rmdir "%TEST_DIR%"
if errorlevel 1 goto failed

del "%OUT%" "%EXPECTED%" >nul 2>nul
echo Flatcore Windows smoke tests passed.
exit /b 0

:failed
echo Flatcore Windows smoke test failed at: %STEP%
if defined TEST_DIR (
    del "%TEST_DIR%\sample.txt" "%TEST_DIR%\copy.txt" "%TEST_DIR%\moved.txt" >nul 2>nul
    rmdir "%TEST_DIR%" >nul 2>nul
)
del "%OUT%" "%EXPECTED%" >nul 2>nul
exit /b 1
