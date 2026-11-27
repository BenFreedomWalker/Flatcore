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
set "PWD_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="%CD%" set "PWD_MATCH=1"
if not defined PWD_MATCH goto failed

set "STEP=hostname output"
"%BIN%" hostname > "%OUT%"
if errorlevel 1 goto failed
for %%A in ("%OUT%") do if "%%~zA"=="0" goto failed

set "TEST_DIR=%TEMP%\flatcore smoke %RANDOM%-%RANDOM%"
set "STEP=mkdir path with spaces"
"%BIN%" mkdir "%TEST_DIR%"
if errorlevel 1 goto failed

set "STEP=mkdir recursive parents"
"%BIN%" mkdir -p "%TEST_DIR%\nested\one\two"
if errorlevel 1 goto failed
if not exist "%TEST_DIR%\nested\one\two" goto failed
set "STEP=mkdir parents already exist"
"%BIN%" mkdir -p "%TEST_DIR%\nested\one\two"
if errorlevel 1 goto failed
set "STEP=mkdir verbose output"
"%BIN%" mkdir -pv "%TEST_DIR%\verbose\deep" > "%OUT%"
if errorlevel 1 goto failed
set "MKDIR_VERBOSE_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="mkdir: created directory '%TEST_DIR%\verbose'" set "MKDIR_VERBOSE_MATCH=1"
if not defined MKDIR_VERBOSE_MATCH goto failed
if not exist "%TEST_DIR%\verbose\deep" goto failed
set "STEP=mkdir mode option accepted"
"%BIN%" mkdir --mode=700 "%TEST_DIR%\mode-dir"
if errorlevel 1 goto failed
if not exist "%TEST_DIR%\mode-dir" goto failed
set "STEP=mkdir help and version"
"%BIN%" mkdir --help > "%OUT%"
if errorlevel 1 goto failed
set "MKDIR_HELP_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Usage: mkdir [OPTION]... DIRECTORY..." set "MKDIR_HELP_MATCH=1"
if not defined MKDIR_HELP_MATCH goto failed
"%BIN%" mkdir --version > "%OUT%"
if errorlevel 1 goto failed
set "MKDIR_VERSION_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Flatcore mkdir 0.1" set "MKDIR_VERSION_MATCH=1"
if not defined MKDIR_VERSION_MATCH goto failed
set "STEP=mkdir invalid mode"
"%BIN%" mkdir -m 888 "%TEST_DIR%\bad-mode" >nul 2>nul
if not errorlevel 1 goto failed

set "STEP=create sample file"
> "%TEST_DIR%\sample.txt" echo alpha
if not exist "%TEST_DIR%\sample.txt" goto failed
> "%TEST_DIR%\second.txt" echo beta
type nul > "%TEST_DIR%\empty.txt"

set "STEP=stat regular file metadata"
"%BIN%" stat "%TEST_DIR%\sample.txt" > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Size: 7 bytes" "%OUT%" >nul
if errorlevel 1 goto failed
findstr /c:"Type: regular file" "%OUT%" >nul
if errorlevel 1 goto failed
findstr /c:"Hidden: no" "%OUT%" >nul
if errorlevel 1 goto failed
findstr /c:"Native hidden: no" "%OUT%" >nul
if errorlevel 1 goto failed
set "STEP=stat dotfile basename detection"
> "%TEST_DIR%\.stat-hidden" echo hidden
"%BIN%" stat "%TEST_DIR%\.stat-hidden" > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Hidden: yes" "%OUT%" >nul
if errorlevel 1 goto failed
findstr /c:"Native hidden: no" "%OUT%" >nul
if errorlevel 1 goto failed
set "STEP=stat nested dotfile basename detection"
mkdir "%TEST_DIR%\stat-hidden-dir"
> "%TEST_DIR%\stat-hidden-dir\.nested-hidden" echo hidden
> "%TEST_DIR%\stat-hidden-dir\visible" echo visible
"%BIN%" stat "%TEST_DIR%\stat-hidden-dir\.nested-hidden" > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Hidden: yes" "%OUT%" >nul
if errorlevel 1 goto failed
set "STEP=stat visible file inside hidden directory"
"%BIN%" stat "%TEST_DIR%\stat-hidden-dir\visible" > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Hidden: no" "%OUT%" >nul
if errorlevel 1 goto failed
set "STEP=stat hidden directory with trailing separator"
mkdir "%TEST_DIR%\.stat-hidden-dir"
"%BIN%" stat "%TEST_DIR%\.stat-hidden-dir/" > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Hidden: yes" "%OUT%" >nul
if errorlevel 1 goto failed
set "STEP=stat read-only attribute"
> "%TEST_DIR%\readonly.txt" echo readonly
attrib +R "%TEST_DIR%\readonly.txt"
"%BIN%" stat "%TEST_DIR%\readonly.txt" > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Read-only: yes" "%OUT%" >nul
if errorlevel 1 goto failed
attrib -R "%TEST_DIR%\readonly.txt"
set "STEP=stat native Windows hidden attribute"
> "%TEST_DIR%\native-hidden.txt" echo hidden
attrib +H "%TEST_DIR%\native-hidden.txt"
"%BIN%" stat "%TEST_DIR%\native-hidden.txt" > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Hidden: no" "%OUT%" >nul
if errorlevel 1 goto failed
findstr /c:"Native hidden: yes" "%OUT%" >nul
if errorlevel 1 goto failed
attrib -H "%TEST_DIR%\native-hidden.txt"
set "STEP=stat directory metadata"
"%BIN%" stat "%TEST_DIR%" > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Type: directory" "%OUT%" >nul
if errorlevel 1 goto failed
set "STEP=stat rejects missing paths"
"%BIN%" stat "%TEST_DIR%\missing-stat-file" >nul 2>nul
if not errorlevel 1 goto failed

set "STEP=touch creates files and accepts multiple operands"
"%BIN%" touch "%TEST_DIR%\touch-created.txt"
if errorlevel 1 goto failed
if not exist "%TEST_DIR%\touch-created.txt" goto failed
"%BIN%" touch "%TEST_DIR%\touch-one.txt" "%TEST_DIR%\touch-two.txt"
if errorlevel 1 goto failed
if not exist "%TEST_DIR%\touch-one.txt" goto failed
if not exist "%TEST_DIR%\touch-two.txt" goto failed
set "STEP=touch clustered timestamp flags"
"%BIN%" touch -a "%TEST_DIR%\touch-created.txt"
if errorlevel 1 goto failed
"%BIN%" touch -m "%TEST_DIR%\touch-created.txt"
if errorlevel 1 goto failed
"%BIN%" touch -am "%TEST_DIR%\touch-created.txt"
if errorlevel 1 goto failed
set "STEP=touch reference timestamps"
> "%TEST_DIR%\touch-reference.txt" echo reference
> "%TEST_DIR%\touch-reference-target.txt" echo target
"%BIN%" touch -r "%TEST_DIR%\touch-reference.txt" "%TEST_DIR%\touch-reference-target.txt"
if errorlevel 1 goto failed
set "STEP=touch rejects a missing reference"
"%BIN%" touch -r "%TEST_DIR%\missing-touch-reference.txt" "%TEST_DIR%\touch-reference-target.txt" >nul 2>nul
if not errorlevel 1 goto failed
set "STEP=touch no-create options"
"%BIN%" touch -c "%TEST_DIR%\missing-touch.txt"
if errorlevel 1 goto failed
if exist "%TEST_DIR%\missing-touch.txt" goto failed
"%BIN%" touch --no-create "%TEST_DIR%\missing-touch-long.txt"
if errorlevel 1 goto failed
if exist "%TEST_DIR%\missing-touch-long.txt" goto failed
set "STEP=touch help and version"
"%BIN%" touch --help > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"--no-create" "%OUT%" >nul
if errorlevel 1 goto failed
"%BIN%" touch --version > "%OUT%"
if errorlevel 1 goto failed
findstr /c:"Flatcore touch 0.1" "%OUT%" >nul
if errorlevel 1 goto failed
set "STEP=touch rejects invalid option"
"%BIN%" touch --not-a-real-option >nul 2>nul
if not errorlevel 1 goto failed
set "STEP=touch continues after an operand error"
"%BIN%" touch "%TEST_DIR%\missing-parent\child.txt" "%TEST_DIR%\touch-after-error.txt" >nul 2>nul
if not errorlevel 1 goto failed
if not exist "%TEST_DIR%\touch-after-error.txt" goto failed
set "STEP=touch requires an operand"
"%BIN%" touch >nul 2>nul
if not errorlevel 1 goto failed

set "STEP=cat stdin redirection"
"%BIN%" cat < "%TEST_DIR%\sample.txt" > "%OUT%"
if errorlevel 1 goto failed
fc /b "%TEST_DIR%\sample.txt" "%OUT%" >nul
if errorlevel 1 goto failed

set "STEP=cat multiple operands"
copy /b "%TEST_DIR%\sample.txt"+"%TEST_DIR%\second.txt" "%EXPECTED%" >nul
"%BIN%" cat "%TEST_DIR%\sample.txt" "%TEST_DIR%\second.txt" > "%OUT%"
if errorlevel 1 goto failed
fc /b "%EXPECTED%" "%OUT%" >nul
if errorlevel 1 goto failed

set "STEP=cat dash stdin operand"
"%BIN%" cat - < "%TEST_DIR%\second.txt" > "%OUT%"
if errorlevel 1 goto failed
fc /b "%TEST_DIR%\second.txt" "%OUT%" >nul
if errorlevel 1 goto failed

set "STEP=cat empty file"
"%BIN%" cat "%TEST_DIR%\empty.txt" > "%OUT%"
if errorlevel 1 goto failed
for %%A in ("%OUT%") do if not "%%~zA"=="0" goto failed

set "STEP=cat continues after missing operand"
"%BIN%" cat "%TEST_DIR%\missing.txt" "%TEST_DIR%\sample.txt" > "%OUT%"
if not errorlevel 1 goto failed
fc /b "%TEST_DIR%\sample.txt" "%OUT%" >nul
if errorlevel 1 goto failed

set "STEP=head output"
"%BIN%" head "%TEST_DIR%\sample.txt" > "%OUT%"
if errorlevel 1 goto failed
> "%EXPECTED%" echo alpha
fc /b "%OUT%" "%EXPECTED%" >nul
if errorlevel 1 goto failed

set "STEP=ls directory and option coverage"
> "%TEST_DIR%\.hidden" echo hidden
> "%TEST_DIR%\visible" echo visible
set "STEP=ls default directory listing"
"%BIN%" ls "%TEST_DIR%" > "%OUT%"
if errorlevel 1 goto failed
set "LS_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="visible" set "LS_MATCH=1"
if not defined LS_MATCH goto failed
set "LS_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"==".hidden" set "LS_MATCH=1"
if defined LS_MATCH goto failed
set "STEP=ls almost-all option"
"%BIN%" ls -A "%TEST_DIR%" > "%OUT%"
if errorlevel 1 goto failed
set "LS_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"==".hidden" set "LS_MATCH=1"
if not defined LS_MATCH goto failed
set "STEP=ls long and all options"
"%BIN%" ls -la "%TEST_DIR%" > "%OUT%"
if errorlevel 1 goto failed
set "LS_MATCH="
for /f "usebackq tokens=3*" %%A in ("%OUT%") do if "%%A"==".hidden" set "LS_MATCH=1"
if not defined LS_MATCH goto failed
set "STEP=ls directory operand option"
"%BIN%" ls -d "%TEST_DIR%" > "%OUT%"
if errorlevel 1 goto failed
set "LS_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="%TEST_DIR%" set "LS_MATCH=1"
if not defined LS_MATCH goto failed
set "STEP=ls multiple file operands"
"%BIN%" ls "%TEST_DIR%\sample.txt" "%TEST_DIR%\second.txt" > "%OUT%"
if errorlevel 1 goto failed
set "LS_SAMPLE="
set "LS_SECOND="
for /f "usebackq delims=" %%A in ("%OUT%") do (
    if "%%A"=="%TEST_DIR%\sample.txt" set "LS_SAMPLE=1"
    if "%%A"=="%TEST_DIR%\second.txt" set "LS_SECOND=1"
)
if not defined LS_SAMPLE goto failed
if not defined LS_SECOND goto failed
set "STEP=ls version option"
"%BIN%" ls --version > "%OUT%"
if errorlevel 1 goto failed
set "LS_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Flatcore ls 0.1" set "LS_MATCH=1"
if not defined LS_MATCH goto failed
set "STEP=ls invalid option"
"%BIN%" ls --not-a-real-option >nul 2>nul
if not errorlevel 1 goto failed

set "STEP=cp help option"
"%BIN%" cp --help > "%OUT%"
findstr /b /c:"Usage: cp " "%OUT%" >nul
if errorlevel 1 goto failed

set "STEP=mv version option"
"%BIN%" mv --version > "%OUT%"
if errorlevel 1 goto failed
set "MV_VERSION_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Flatcore mv 0.1" set "MV_VERSION_MATCH=1"
if not defined MV_VERSION_MATCH goto failed

set "STEP=copy file"
"%BIN%" cp "%TEST_DIR%\sample.txt" "%TEST_DIR%\copy.txt"
if errorlevel 1 goto failed

set "STEP=cp identical source and destination"
"%BIN%" cp -- "%TEST_DIR%\sample.txt" "%TEST_DIR%\sample.txt" >nul 2>nul
if not errorlevel 1 goto failed

set "STEP=move file"
"%BIN%" mv "%TEST_DIR%\copy.txt" "%TEST_DIR%\moved.txt"
if errorlevel 1 goto failed

set "STEP=move replaces existing destination"
> "%TEST_DIR%\move-destination.txt" echo old destination
> "%TEST_DIR%\move-source.txt" echo new source
"%BIN%" mv -- "%TEST_DIR%\move-source.txt" "%TEST_DIR%\move-destination.txt"
if errorlevel 1 goto failed
findstr /x /c:"new source" "%TEST_DIR%\move-destination.txt" >nul
if errorlevel 1 goto failed
if exist "%TEST_DIR%\move-source.txt" goto failed

set "STEP=cat file"
"%BIN%" cat "%TEST_DIR%\moved.txt" >nul
if errorlevel 1 goto failed

set "STEP=basename output"
"%BIN%" basename "%TEST_DIR%\sample.txt" > "%OUT%"
set "BASENAME_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="sample.txt" set "BASENAME_MATCH=1"
if not defined BASENAME_MATCH goto failed

set "STEP=basename drive root"
"%BIN%" basename C:\ > "%OUT%"
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

set "STEP=dirname relative path"
"%BIN%" dirname "sample.txt" > "%OUT%"
set "DIRNAME_DOT_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="." set "DIRNAME_DOT_MATCH=1"
if not defined DIRNAME_DOT_MATCH goto failed

set "STEP=rm help and version"
"%BIN%" rm --help > "%OUT%"
if errorlevel 1 goto failed
set "RM_HELP_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Usage: rm [OPTION]... FILE..." set "RM_HELP_MATCH=1"
if not defined RM_HELP_MATCH goto failed
"%BIN%" rm --version > "%OUT%"
if errorlevel 1 goto failed
set "RM_VERSION_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Flatcore rm 0.1" set "RM_VERSION_MATCH=1"
if not defined RM_VERSION_MATCH goto failed
set "STEP=rm force ignores missing path"
"%BIN%" rm -f "%TEST_DIR%\missing-rm-file"
if errorlevel 1 goto failed
set "STEP=rm removes sample file"
"%BIN%" rm "%TEST_DIR%\sample.txt"
if errorlevel 1 goto failed
set "STEP=rm verbose output"
> "%TEST_DIR%\rm-verbose.txt" echo verbose
"%BIN%" rm -v "%TEST_DIR%\rm-verbose.txt" > "%OUT%"
if errorlevel 1 goto failed
set "RM_VERBOSE_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="rm: removed '%TEST_DIR%\rm-verbose.txt'" set "RM_VERBOSE_MATCH=1"
if not defined RM_VERBOSE_MATCH goto failed
set "STEP=rm removes empty directory with -d"
mkdir "%TEST_DIR%\rm-empty"
"%BIN%" rm -dv "%TEST_DIR%\rm-empty" > "%OUT%"
if errorlevel 1 goto failed
if exist "%TEST_DIR%\rm-empty" goto failed
set "STEP=rm -d rejects non-empty directory"
mkdir "%TEST_DIR%\rm-nonempty"
> "%TEST_DIR%\rm-nonempty\keep.txt" echo keep
"%BIN%" rm -d "%TEST_DIR%\rm-nonempty" >nul 2>nul
if not errorlevel 1 goto failed
if not exist "%TEST_DIR%\rm-nonempty\keep.txt" goto failed
del "%TEST_DIR%\rm-nonempty\keep.txt"
"%BIN%" rm -d "%TEST_DIR%\rm-nonempty"
if errorlevel 1 goto failed

set "STEP=remove moved file"
"%BIN%" rm "%TEST_DIR%\moved.txt"
if errorlevel 1 goto failed

del "%TEST_DIR%\touch-created.txt" "%TEST_DIR%\touch-one.txt" "%TEST_DIR%\touch-two.txt" "%TEST_DIR%\touch-reference.txt" "%TEST_DIR%\touch-reference-target.txt" "%TEST_DIR%\touch-after-error.txt" "%TEST_DIR%\move-destination.txt" "%TEST_DIR%\second.txt" "%TEST_DIR%\empty.txt" "%TEST_DIR%\.hidden" "%TEST_DIR%\.stat-hidden" "%TEST_DIR%\visible" "%TEST_DIR%\readonly.txt" "%TEST_DIR%\native-hidden.txt" "%TEST_DIR%\stat-hidden-dir\.nested-hidden" "%TEST_DIR%\stat-hidden-dir\visible" >nul 2>nul
rmdir "%TEST_DIR%\stat-hidden-dir" >nul 2>nul
rmdir "%TEST_DIR%\.stat-hidden-dir" >nul 2>nul

set "STEP=rmdir parents and verbose output"
pushd "%TEST_DIR%"
if errorlevel 1 goto failed
"%BIN%" mkdir -p rmdir-chain\one\two
if errorlevel 1 goto rmdir_parent_failed
"%BIN%" rmdir -pv rmdir-chain\one\two > "%OUT%"
if errorlevel 1 goto rmdir_parent_failed
set "RMDIR_VERBOSE_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="rmdir: removed 'rmdir-chain\one\two'" set "RMDIR_VERBOSE_MATCH=1"
if not defined RMDIR_VERBOSE_MATCH goto rmdir_parent_failed
if exist rmdir-chain goto rmdir_parent_failed
popd

set "STEP=rmdir help and version"
"%BIN%" rmdir --help > "%OUT%"
if errorlevel 1 goto failed
set "RMDIR_HELP_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Usage: rmdir [OPTION]... DIRECTORY..." set "RMDIR_HELP_MATCH=1"
if not defined RMDIR_HELP_MATCH goto failed
"%BIN%" rmdir --version > "%OUT%"
if errorlevel 1 goto failed
set "RMDIR_VERSION_MATCH="
for /f "usebackq delims=" %%A in ("%OUT%") do if "%%A"=="Flatcore rmdir 0.1" set "RMDIR_VERSION_MATCH=1"
if not defined RMDIR_VERSION_MATCH goto failed
set "STEP=rmdir invalid option"
"%BIN%" rmdir --not-a-real-option >nul 2>nul
if not errorlevel 1 goto failed

set "STEP=rmdir rejects non-empty directory"
mkdir "%TEST_DIR%\rmdir-nonempty"
> "%TEST_DIR%\rmdir-nonempty\file.txt" echo keep
"%BIN%" rmdir "%TEST_DIR%\rmdir-nonempty" >nul 2>nul
if not errorlevel 1 goto failed
if not exist "%TEST_DIR%\rmdir-nonempty\file.txt" goto failed
del "%TEST_DIR%\rmdir-nonempty\file.txt"
"%BIN%" rmdir "%TEST_DIR%\rmdir-nonempty"
if errorlevel 1 goto failed

set "STEP=remove mkdir test tree"
"%BIN%" rmdir "%TEST_DIR%\nested\one\two"
if errorlevel 1 goto failed
"%BIN%" rmdir "%TEST_DIR%\nested\one"
if errorlevel 1 goto failed
"%BIN%" rmdir "%TEST_DIR%\nested"
if errorlevel 1 goto failed
"%BIN%" rmdir "%TEST_DIR%\verbose\deep"
if errorlevel 1 goto failed
"%BIN%" rmdir "%TEST_DIR%\verbose"
if errorlevel 1 goto failed
"%BIN%" rmdir "%TEST_DIR%\mode-dir"
if errorlevel 1 goto failed

set "STEP=rmdir test directory"
"%BIN%" rmdir "%TEST_DIR%"
if errorlevel 1 goto failed

del "%OUT%" "%EXPECTED%" >nul 2>nul
echo Flatcore Windows smoke tests passed.
exit /b 0

:rmdir_parent_failed
popd
goto failed

:failed
echo Flatcore Windows smoke test failed at: %STEP%
if defined TEST_DIR (
    attrib -R "%TEST_DIR%\readonly.txt" >nul 2>nul
    attrib -H "%TEST_DIR%\native-hidden.txt" >nul 2>nul
    del "%TEST_DIR%\touch-created.txt" "%TEST_DIR%\touch-one.txt" "%TEST_DIR%\touch-two.txt" "%TEST_DIR%\touch-reference.txt" "%TEST_DIR%\touch-reference-target.txt" "%TEST_DIR%\move-destination.txt" "%TEST_DIR%\sample.txt" "%TEST_DIR%\second.txt" "%TEST_DIR%\empty.txt" "%TEST_DIR%\copy.txt" "%TEST_DIR%\moved.txt" "%TEST_DIR%\.hidden" "%TEST_DIR%\.stat-hidden" "%TEST_DIR%\visible" "%TEST_DIR%\readonly.txt" "%TEST_DIR%\native-hidden.txt" "%TEST_DIR%\stat-hidden-dir\.nested-hidden" "%TEST_DIR%\stat-hidden-dir\visible" >nul 2>nul
    rmdir /s /q "%TEST_DIR%" >nul 2>nul
)
del "%OUT%" "%EXPECTED%" >nul 2>nul
exit /b 1
