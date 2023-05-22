@REM v3.5.2 This script uninstall patch for any exe.
@REM Usage: drag target exe to this script on explorer or
@REM uninstall1 D:\xxx\xxx.exe

@echo off
cd /d %~dp0

set instDir=%~dp1
set instApp=%~nx1

echo Uninstalling `%instApp%` in `%instDir%` ...

set realAppPrefix=wrl-

set X86HASH=
set X64HASH=
for /f "delims=" %%i in ('wsls-hash "x86/wsls-shell.exe"') do set X86HASH=%%i
for /f "delims=" %%i in ('wsls-hash "x64/wsls-shell.exe"') do set X64HASH=%%i

echo X86HASH=%X86HASH%
echo X64HASH=%X64HASH%

REM clear errorlevel before starting install patch
set errorlevel=

REM uninstall patch for app(xxx.exe)
call :UninstallPatch %instDir% %instApp% nil

REM exit uninstall.bat script
:L_exit
echo Exiting uninstall.bat...&ping /n 2 127.0.1 >nul
goto :eof

REM --------------------- the InstalPatch subprocedure ----------------
REM <params>
REM  <instDir>The target .exe directory</instDir>
REM  <instApp>The target .exe</instApp>
REM  <arch>The arch of target, optional: if not specific, will auto detect</arch>
REM  <redApp>The redirect App, don't use original of ndk, such as gnumake.exe</redApp>
REM </params>
:UninstallPatch
set instDir=%1
set instApp=%2
set arch=%3
set redApp=%4

REM -- clear parameters
if "%arch%"=="nil" set arch=
if "%redApp%"=="nil" set redApp=

REM if the arch undefined or nil, detect it auto
setlocal ENABLEDELAYEDEXPANSION
set errorlevel=

REM Notes: ensure always detect arch from real app, this will auto fix previous release 3.4.x arch mismatch bug
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

REM -- print parameters
REM echo instDir="%instDir%"
REM echo instApp="%instApp%"
REM echo arch="%arch%"
REM echo redApp="%redApp%"

REM custom such as gnumake.exe
if defined redApp (
  if exist "%instDir%\%redApp%" del del /q /s /f "%instDir%\%redApp%"
  if exist "%instDir%\%instApp%.bridge" del "%instDir%\%instApp%.bridge"
)

set instHash=
for /f "delims=" %%i in ('wsls-hash "%instDir%\%instApp%"') do set instHash=%%i

if "%arch%"=="x64" (
  if not "%instHash%"=="%X64HASH%" goto :L_not_installed
) else (
  if not "%instHash%"=="%X86HASH%" goto :L_not_installed
)

echo Uninstalling patch for %instApp%(%arch%)...

REM --- perform uninstall
if exist "%instDir%\%realAppPrefix%%instApp%" (
  del /q /f "%instDir%\%instApp%"
  move /Y "%instDir%\%realAppPrefix%%instApp%" "%instDir%\%instApp%"
)

echo Uninstall patch for %instApp%(%arch%) succeed.
goto :eof

:L_not_installed
echo "The patch for %instApp%(%arch%) not installed."
goto :eof
