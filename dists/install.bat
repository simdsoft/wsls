@rem This script install patch for android ndk(x64) and android sdk tools's .exe
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

rem Cleanup old version stubs
del /s /f /q "%ndkRoot%\*.bridge" 2>nul
del /s /f /q "%ndkRoot%\*wsLongPaths.dll" 2>nul
del /s /f /q "%ndkRoot%\*wow64helper.exe" 2>nul

set X86HASH=
set X64HASH=
for /f "delims=" %%i in ('wsls-hash "x86/wsls-core.exe"') do set X86HASH=%%i
for /f "delims=" %%i in ('wsls-hash "x64/wsls-core.exe"') do set X64HASH=%%i

echo X86HASH=%X86HASH%
echo X64HASH=%X64HASH%

rem Install wsls core binaires
if exist %WINDIR%\SysWow64\ (
  copy /y x64\wsls-copy.exe %WINDIR%\System32\
  copy /y x64\wsls-del.exe %WINDIR%\System32\
  copy /y x64\wsls-md.exe %WINDIR%\System32\
  copy /y x64\wow64helper.exe %WINDIR%\System32\
  copy /y x64\wsLongPaths.dll %WINDIR%\System32\
  
  copy /y x86\wsls-copy.exe %WINDIR%\SysWow64\
  copy /y x86\wsls-del.exe %WINDIR%\SysWow64\
  copy /y x86\wsls-md.exe %WINDIR%\SysWow64\
  copy /y x86\wow64helper.exe %WINDIR%\SysWow64\
  copy /y x86\wsLongPaths.dll %WINDIR%\SysWow64\
) else (
  copy /y x86\wsls-copy.exe %WINDIR%\System32\
  copy /y x86\wsls-del.exe %WINDIR%\System32\
  copy /y x86\wsls-md.exe %WINDIR%\System32\
  copy /y x86\wow64helper.exe %WINDIR%\System32\
  copy /y x86\wsLongPaths.dll %WINDIR%\System32\
)

rem detect ndk revision
set ndkVer=
for /f "usebackq tokens=2,3* delims=^= " %%i in ("%ndkRoot%\source.properties") do set ndkVer=%%i

echo Android NDK = %ndkVer%

rem get major version
set ndkMajorVer=
for /f "tokens=1,2* delims=." %%i in ("%ndkVer%") do set ndkMajorVer=%%i

echo Android NDK major version is: %ndkMajorVer%

rem patching echo
wsls-copy %arch%\wsls-echo.exe "%ndkRoot%\prebuilt\windows-x86_64\bin\wsls-echo.exe"

rem clear errorlevel before starting install patch
set errorlevel=

rem patching make.exe & cmp.exe
call :InstallPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" make.exe x64 gnumake.exe
call :InstallPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" cmp.exe x64

rem patching LLVM clang
call :InstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" clang++.exe x64
call :InstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" clang.exe x64

rem patching LLVM for build armv7
call :InstallPatchForLLVM arm-linux-androideabi

rem patching LLVM for build arm64
call :InstallPatchForLLVM aarch64-linux-android

rem patching gcc armv7
call :InstallPatchForGCC arm-linux-androideabi

rem patching gcc armv8a
call :InstallPatchForGCC aarch64-linux-android

rem patching android sdk tools
if defined sdkRoot call :InstallPatchForSdkTools

rem exit install.bat script
:L_exit
echo Exiting install.bat...&ping /n 2 127.0.1 >nul
goto :eof

rem ---------------- the InstallPatchForSdkTools subprocedure ------------
:InstallPatchForSdkTools

rem -- cmake
FOR /f "delims=" %%i IN ('dir "%sdkRoot%\cmake" /ad /b') DO (
  for /f "delims=" %%j in ('dir "%sdkRoot%\cmake\%%i\bin" /a-d /b ^| findstr /v "ndk-"') do (
    call :InstallPatch "%sdkRoot%\cmake\%%i\bin" %%j
  )
)

rem -- build-tools aidl.exe
FOR /f "delims=" %%i IN ('dir "%sdkRoot%\build-tools" /ad /b') DO (
  call :InstallPatch "%sdkRoot%\build-tools\%%i" aidl.exe
)

goto :eof

rem ---------------- the InstallPatchForLLVM subprocedure ------------
:InstallPatchForLLVM
set HOST=%1
call :InstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" %HOST%-ar.exe x64
call :InstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" %HOST%-ld.exe x64
call :InstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" %HOST%-ranlib.exe x64
if %ndkMajorVer% GTR 18 (
  call :InstallPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\%HOST%\bin" ld.exe x64
)
goto :eof


rem ---------------- the InstallPatchForGCC subprocedure ------------

:InstallPatchForGCC
set HOST=%1
call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-g++.exe x64
call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-gcc.exe x64
call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-ar.exe x64
call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-ranlib.exe x64

call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" ld.exe x64
call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" as.exe x64
call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" ar.exe x64

rem skip libexec when ndk > 17, since ndk-r18, libexec removed
if %ndkMajorVer% GTR 17 goto :eof

call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" cc1.exe x64
call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" cc1plus.exe x64
call :InstallPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" collect2.exe x64
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

rem if not defined arch detect it auto
setlocal ENABLEDELAYEDEXPANSION
set errorlevel=
if not defined arch (
  call wsls-arch.exe "%instDir%\%instApp%"
  if !errorlevel! GTR 0 set arch=x!errorlevel!
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
  wsls-copy %arch%\%redApp% "%instDir%\%redApp%"
  if exist %arch%\%instApp%.bridge wsls-copy %arch%\%instApp%.bridge "%instDir%\%instApp%.bridge"
)

set instHash=
for /f "delims=" %%i in ('wsls-hash "%instDir%\%instApp%"') do set instHash=%%i

if "%arch%"=="x64" (
  if "%instHash%"=="%X64HASH%" goto :L_installed
) else (
  if "%instHash%"=="%X86HASH%" goto :L_installed
)

echo Installing patch for %instApp%...

rem --- perform install
rem make a copy of original xxx.exe to ndk-xxx.exe
if not exist "%instDir%\ndk-%instApp%" wsls-copy "%instDir%\%instApp%" "%instDir%\ndk-%instApp%"

wsls-copy %arch%\wsls-core.exe "%instDir%\%instApp%"

echo Installing patch for %instApp%(%arch%) succeed.
goto :eof

:L_installed
echo The patch for %instApp%(%arch%) already installed.
goto :eof
