@rem v3.5.3 This script install patch for android ndk(x64) and android sdk tools's .exe
@echo off

set myDir=%~dp0

cd /d "%myDir%"

set instDir=%~dp1
set instApp=%~nx1

set realAppPrefix=wrl-

echo [wsls] Installing ...

set X86HASH=
set X64HASH=
for /f "delims=" %%i in ('wsls-hash "x86/wsls-shell.exe"') do set X86HASH=%%i
for /f "delims=" %%i in ('wsls-hash "x64/wsls-shell.exe"') do set X64HASH=%%i

echo X86HASH=%X86HASH%
echo X64HASH=%X64HASH%

if "%WSLS_DIST%"=="" (
  echo "Setting env var WSLS_DIST=%myDir%"
  powershell "[Environment]::SetEnvironmentVariable('WSLS_DIST', '%myDir%', 'User')"
  echo "Set env var WSLS_DIST=%myDir% done"
)

set PATH=%myDir%;%PATH%

rem clear errorlevel before starting install patch
set errorlevel=

rem patching make.exe & cmp.exe
call :InstallPatch %instDir% %instApp%

REM exit install.bat script
:L_exit
echo Exiting install.bat...&ping /n 2 127.0.1 >nul
goto :eof

rem --------------------- the InstalPatch subprocedure ----------------
rem <params>
rem  <instDir>The target .exe directory</instDir>
rem  <instApp>The target .exe</instApp>
rem  <arch>The arch of target, optional: if not specific, will auto detect</arch>
rem  <redApp>The redirect App, don't use original of ndk, such as gnumake.exe</redApp>
rem </params>
:InstallPatch
set instDir=%1
set instApp=%2
set arch=%3
set redApp=%4

rem -------- check parameters ------

rem -- clear parameter value if nil
if "%arch%"=="nil" set arch=
if "%redApp%"=="nil" set redApp=

rem if the arch undefined or nil, detect it auto
setlocal ENABLEDELAYEDEXPANSION
set errorlevel=

rem Notes: ensure always detect arch from real app, this will auto fix previous release 3.4.x arch mismatch bug
if not defined arch (
  if exist "%instDir%\%realAppPrefix%%instApp%" (
    call wsls-arch.exe "%instDir%\%realAppPrefix%%instApp%"
    if !errorlevel! GTR 0 set arch=x!errorlevel!
  ) else (
    call wsls-arch.exe "%instDir%\%instApp%"
    if !errorlevel! GTR 0 set arch=x!errorlevel!
  )
)

set errorlevel=
setlocal DISABLEDELAYEDEXPANSION

if not "%arch%"=="x86" (
  if not "%arch%"=="x64" (
    echo Skipping target %instDir%\%instApp% with arch '%arch%', valid arch list: x86,x64  && goto :eof
  )
)

rem -- print parameters
rem echo instDir="%instDir%"
rem echo instApp="%instApp%"
rem echo arch="%arch%"
rem echo redApp="%redApp%"

rem custom such as gnumake.exe
if defined redApp (
  copy /y %arch%\%redApp% "%instDir%\%redApp%"
  if exist %arch%\%instApp%.bridge copy /y %arch%\%instApp%.bridge "%instDir%\%instApp%.bridge"
)

set instHash=
for /f "delims=" %%i in ('wsls-hash "%instDir%\%instApp%"') do set instHash=%%i

if "%arch%"=="x64" (
  if "%instHash%"=="%X64HASH%" goto :L_installed
) else (
  if "%instHash%"=="%X86HASH%" goto :L_installed
)

echo Installing patch for %instApp%(%arch%)...

rem --- perform install
rem make a copy of original xxx.exe to wrl-xxx.exe
if not exist "%instDir%\%realAppPrefix%%instApp%" copy /y "%instDir%\%instApp%" "%instDir%\%realAppPrefix%%instApp%"

copy /y %arch%\wsls-shell.exe "%instDir%\%instApp%"

echo Install patch for %instApp%(%arch%) succeed.
goto :eof

:L_installed
echo The patch for %instApp%(%arch%) already installed.
goto :eof
