@REM v3.5 This script uninstall patch for android ndk(x64) and android sdk tools's .exe
@echo off
cd /d %~dp0

set ndkRoot=%1
if not defined ndkRoot set ndkRoot=%ANDROID_NDK%
if not defined ndkRoot echo Please specific ANDROID_NDK! && goto :L_exit
if not exist "%ndkRoot%" echo No valid ANDROID_NDK directory specificed! && goto :L_exit

set sdkRoot=%2
if not defined sdkRoot set sdkRoot=%ANDROID_SDK%
if not exist "%sdkRoot%" echo No valid ANDROID_SDK directory specificed! & set sdkRoot=

echo ndkRoot=%ndkRoot%
echo sdkRoot=%sdkRoot%

set realAppPrefix=wrl-

REM Cleanup old version stubs
del /s /f /q "%ndkRoot%\*.bridge" 2>nul
del /s /f /q "%ndkRoot%\*wsLongPaths.dll" 2>nul
del /s /f /q "%ndkRoot%\*wow64helper.exe" 2>nul

set X86HASH=
set X64HASH=
for /f "delims=" %%i in ('wsls-hash "x86/wsls-shell.exe"') do set X86HASH=%%i
for /f "delims=" %%i in ('wsls-hash "x64/wsls-shell.exe"') do set X64HASH=%%i

echo X86HASH=%X86HASH%
echo X64HASH=%X64HASH%

REM Install wsls core binaires
REM del /q %WINDIR%\System32\wsLongPaths.dll 2>nul
REM if exist %WINDIR%\SysWow64\ (
REM   del /q %WINDIR%\SysWow64\wsLongPaths.dll 2>nul
  
REM   copy /y x64\wsls-copy.exe %WINDIR%\System32\
REM   copy /y x64\wsls-del.exe %WINDIR%\System32\
REM   copy /y x64\wsls-md.exe %WINDIR%\System32\
REM   copy /y x64\wow64helper.exe %WINDIR%\System32\
REM   copy /y x64\wsls-core.dll %WINDIR%\System32\
  
REM   copy /y x86\wsls-copy.exe %WINDIR%\SysWow64\
REM   copy /y x86\wsls-del.exe %WINDIR%\SysWow64\
REM   copy /y x86\wsls-md.exe %WINDIR%\SysWow64\
REM   copy /y x86\wow64helper.exe %WINDIR%\SysWow64\
REM   copy /y x86\wsls-core.dll %WINDIR%\SysWow64\
REM ) else (
REM   copy /y x86\wsls-copy.exe %WINDIR%\System32\
REM   copy /y x86\wsls-del.exe %WINDIR%\System32\
REM   copy /y x86\wsls-md.exe %WINDIR%\System32\
REM   copy /y x86\wow64helper.exe %WINDIR%\System32\
REM   copy /y x86\wsls-core.dll %WINDIR%\System32\
REM )

REM detect ndk revision
set ndkVer=
for /f "usebackq tokens=2,3* delims=^= " %%i in ("%ndkRoot%\source.properties") do set ndkVer=%%i

echo Android NDK = %ndkVer%

REM get major version
set ndkMajorVer=
for /f "tokens=1,2* delims=." %%i in ("%ndkVer%") do set ndkMajorVer=%%i

echo Android NDK major version is: %ndkMajorVer%

REM patching echo
wsls-del "%ndkRoot%\prebuilt\windows-x86_64\bin\wsls-echo.exe"

REM clear errorlevel before starting install patch
set errorlevel=

REM patching make.exe & cmp.exe
call :UninstallPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" make.exe x64 gnumake.exe
call :UninstallPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" cmp.exe x64

REM patching LLVM clang
call :UninstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" clang++.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" clang.exe x64

REM patching LLVM for build armv7
call :UninstallPatchForLLVM arm-linux-androideabi

REM patching LLVM for build arm64
call :UninstallPatchForLLVM aarch64-linux-android

REM patching gcc armv7
call :UninstallPatchForGCC arm-linux-androideabi

REM patching gcc armv8a
call :UninstallPatchForGCC aarch64-linux-android

REM patching android sdk tools
if defined sdkRoot call :UninstallPatchForSdkTools

REM exit uninstall.bat script
:L_exit
echo Exiting uninstall.bat...&ping /n 2 127.0.1 >nul
goto :eof

REM ---------------- the UninstallPatchForSdkTools subprocedure ------------
:UninstallPatchForSdkTools

REM -- cmake
FOR /f "delims=" %%i IN ('dir "%sdkRoot%\cmake" /ad /b') DO (
  for /f "delims=" %%j in ('dir "%sdkRoot%\cmake\%%i\bin" /a-d /b ^| findstr /v "%realAppPrefix%" ^| findstr /v "ndk-"') do (
    call :UninstallPatch "%sdkRoot%\cmake\%%i\bin" %%j nil
  )
)

REM -- build-tools aidl.exe
FOR /f "delims=" %%i IN ('dir "%sdkRoot%\build-tools" /ad /b') DO (
  call :UninstallPatch "%sdkRoot%\build-tools\%%i" aidl.exe nil
)

goto :eof

REM ---------------- the UninstallPatchForLLVM subprocedure ------------
:UninstallPatchForLLVM
set HOST=%1
call :UninstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" %HOST%-ar.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" %HOST%-ld.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" %HOST%-ranlib.exe x64
if %ndkMajorVer% GTR 18 (
  call :UninstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\%HOST%\bin" ld.exe x64
)
goto :eof


REM ---------------- the UninstallPatchForGCC subprocedure ------------

:UninstallPatchForGCC
set HOST=%1
call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-g++.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-gcc.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-ar.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-ranlib.exe x64

call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" ld.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" as.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" ar.exe x64

REM skip libexec when ndk > 17, since ndk-r18, libexec removed
if %ndkMajorVer% GTR 17 goto :eof

call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" cc1.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" cc1plus.exe x64
call :UninstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" collect2.exe x64
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

REM Upgrade 3.4.1 to 3.5.0, we use new prefix "wrl-" for real app
REM if exist "%instDir%\ndk-%instApp%" move "%instDir%\ndk-%instApp%" "%instDir%\%realAppPrefix%%instApp%"

REM -------- check parameters ------

REM -- clear parameter value if nil
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
  if exist "%instDir%\%redApp%" wsls-del "%instDir%\%redApp%"
  if exist "%instDir%\%instApp%.bridge" wsls-del "%instDir%\%instApp%.bridge"
)

set instHash=
for /f "delims=" %%i in ('wsls-hash "%instDir%\%instApp%"') do set instHash=%%i

if "%arch%"=="x64" (
  if not "%instHash%"=="%X64HASH%" goto :L_not_installed
) else (
  if not "%instHash%"=="%X86HASH%" goto :L_not_installed
)

echo Uninstalling patch for %instApp%...

REM --- perform uninstall
if exist "%instDir%\%realAppPrefix%%instApp%" (
  wsls-del "%instDir%\%instApp%"
  move /Y "%instDir%\%realAppPrefix%%instApp%" "%instDir%\%instApp%"
)

echo Uninstall patch for %instApp%(%arch%) succeed.
goto :eof

:L_not_installed
echo "The patch for %instApp%(%arch%) not installed."
goto :eof
