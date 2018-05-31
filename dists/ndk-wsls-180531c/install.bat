@echo off
pushd %~dp0

set ndkRoot=%1
if not defined ndkRoot set ndkRoot=%ANDROID_NDK_ROOT%
if not defined ndkRoot echo Please specific ANDROID_NDK_ROOT! && goto :L_exit

call :InstPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" make.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin" arm-linux-androideabi-g++.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin" arm-linux-androideabi-gcc.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x" cc1plus.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin" ld.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin" as.exe

goto :L_exit

:InstPatch
set instDir=%1
set instApp=%2
echo Installing patch for %instApp%...

if not exist %instApp%.bridge echo Install patch for %instApp% failed, missing .bridge config file. && goto :eof

fc wsls-core.exe "%instDir%\%instApp%" 1>nul 2>nul
if %errorlevel%==0 echo The patch for %instApp% already installed. && goto :eof

if not exist "%instDir%\ndk-%instApp%" lcopy "%instDir%\%instApp%" "%instDir%\ndk-%instApp%"
lcopy wsls-core.exe "%instDir%\%instApp%"
lcopy %instApp%.bridge "%instDir%\%instApp%.bridge"

lcopy wow64helper.exe "%instDir%\wow64helper.exe"
lcopy wsLongPaths.dll "%instDir%\wsLongPaths.dll"

echo Installing patch for %instApp% succeed.
goto :eof

:L_exit
ping /n 3 127.0.1 >nul
goto :eof
