rem 1. This install script only install patch for android ndk x64 arch tools's .exe.
@echo off
cd /d %~dp0

set arch=x64

set ndkRoot=%1
if not defined ndkRoot set ndkRoot=%ANDROID_NDK%

if not defined ndkRoot echo Please specific ANDROID_NDK! && goto :L_exit

if not exist %ndkRoot% echo The directory not exist! && goto :L_exit

rem Cleanup old version stubs
del /s /f /q "%ndkRoot%\*.bridge" 2>nul
del /s /f /q "%ndkRoot%\*wsLongPaths.dll" 2>nul
del /s /f /q "%ndkRoot%\*wow64helper.exe" 2>nul

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

rem TODO:
rem set sdkRoot=d:\dev\adt\sdk
rem call :InstPatch "%sdkRoot%\build-tools\28.0.3" aidl.exe
rem goto :L_exit

rem patching echo
wsls-copy %arch%\wsls-echo.exe "%ndkRoot%\prebuilt\windows-x86_64\bin\wsls-echo.exe"

rem patching make.exe & cmp.exe
call :InstPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" make.exe gnumake.exe
call :InstPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" cmp.exe

rem patching LLVM clang
call :InstPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" clang++.exe
call :InstPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" clang.exe

rem patching LLVM for build armv7
call :InstallPatchForLLVM arm-linux-androideabi

rem patching LLVM for build arm64
call :InstallPatchForLLVM aarch64-linux-android

rem patching gcc armv7
call :InstallPatchForGCC arm-linux-androideabi

rem patching gcc armv8a
call :InstallPatchForGCC aarch64-linux-android

goto :L_exit

:InstallPatchForLLVM
set HOST=%1
call :InstPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\bin" %HOST%-ar.exe
call :InstPatch "%ndkRoot%\toolchains\llvm\prebuilt\windows-x86_64\%HOST%\bin" ld.exe
goto :eof

rem ----------------------- subprocedure -------------------------------------

:InstallPatchForGCC
set HOST=%1
call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-g++.exe
call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-gcc.exe
call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\bin" %HOST%-ar.exe

call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" ld.exe
call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" as.exe
call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\%HOST%\bin" ar.exe

rem skip libexec when ndk > 17, since ndk-r18, libexec removed
if %ndkMajorVer% GTR 17 goto :eof

call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" cc1.exe
call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" cc1plus.exe
call :InstPatch "%ndkRoot%\toolchains\%HOST%-4.9\prebuilt\windows-x86_64\libexec\gcc\%HOST%\4.9.x" collect2.exe
goto :eof

:InstPatch
set instDir=%1
set instApp=%2
set rediretApp=%3
echo Installing patch for %instApp%...

if defined rediretApp wsls-copy %arch%\%rediretApp% "%instDir%\%rediretApp%"

fc wsls-core.exe "%instDir%\%instApp%" 1>nul 2>nul
if not %errorlevel%==0 goto :L_continue

echo The patch for %instApp% already installed.
goto :L_exit

:L_continue
if not exist "%instDir%\ndk-%instApp%" wsls-copy "%instDir%\%instApp%" "%instDir%\ndk-%instApp%"

wsls-copy %arch%\wsls-core.exe "%instDir%\%instApp%"
if exist %arch%\%instApp%.bridge wsls-copy %arch%\%instApp%.bridge "%instDir%\%instApp%.bridge"

rem wsls-copy %arch%\wow64helper.exe "%instDir%\wow64helper.exe"
rem wsls-copy %arch%\wsLongPaths.dll "%instDir%\wsLongPaths.dll"

echo Installing patch for %instApp% succeed.
goto :eof

:L_exit
ping /n 3 127.0.1 >nul
goto :eof
