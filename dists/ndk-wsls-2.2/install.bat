@echo off
cd /d %~dp0

set ndkRoot=%1
if not defined ndkRoot set ndkRoot=%ANDROID_NDK_ROOT%

if not defined ndkRoot echo Please specific ANDROID_NDK_ROOT! && goto :L_exit

if not exist %ndkRoot% echo The directory not exist! && goto :L_exit

copy /y wsls-copy.exe %WINDIR%\System32\
copy /y wsls-del.exe %WINDIR%\System32\
copy /y wsls-md.exe %WINDIR%\System32\

wsls-copy wsls-echo.exe "%ndkRoot%\prebuilt\windows-x86_64\bin\wsls-echo.exe"

call :InstPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" make.exe gnumake.exe
call :InstPatch "%ndkRoot%\prebuilt\windows-x86_64\bin" cmp.exe

call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin" arm-linux-androideabi-g++.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin" arm-linux-androideabi-gcc.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin" arm-linux-androideabi-ar.exe

call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin" ld.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin" as.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin" ar.exe

call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x" cc1.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x" cc1plus.exe
call :InstPatch "%ndkRoot%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x" collect2.exe

goto :L_exit

:InstPatch
set instDir=%1
set instApp=%2
set rediretApp=%3
echo Installing patch for %instApp%...

if not exist %instApp%.bridge echo Install patch for %instApp% failed, missing .bridge config file. && goto :eof

if defined rediretApp wsls-copy %rediretApp% "%instDir%\%rediretApp%"

fc wsls-core.exe "%instDir%\%instApp%" 1>nul 2>nul
if not %errorlevel%==0 goto :L_continue

fc wsLongPaths.dll "%instDir%\wsLongPaths.dll" 1>nul 2>nul
if not %errorlevel%==0 goto :L_continue

echo The patch for %instApp% already installed.
goto :L_exit

:L_continue
if not exist "%instDir%\ndk-%instApp%" wsls-copy "%instDir%\%instApp%" "%instDir%\ndk-%instApp%"

wsls-copy wsls-core.exe "%instDir%\%instApp%"
wsls-copy %instApp%.bridge "%instDir%\%instApp%.bridge"

wsls-copy wow64helper.exe "%instDir%\wow64helper.exe"
wsls-copy wsLongPaths.dll "%instDir%\wsLongPaths.dll"

echo Installing patch for %instApp% succeed.
goto :eof

:L_exit
ping /n 3 127.0.1 >nul
goto :eof
