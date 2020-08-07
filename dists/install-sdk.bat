rem 1. Install android sdk long path patch.
@echo off
cd /d %~dp0

set sdkRoot=%1
set buildToolsRevision=%2
if not defined sdkRoot set sdkRoot=%ANDROID_SDK%

if not defined sdkRoot echo Please specific ANDROID_SDK! && goto :L_exit

if not exist %sdkRoot% echo The directory not exist! && goto :L_exit

rem Cleanup old version stubs
del /s /f /q "%sdkRoot%\*.bridge" 2>nul
del /s /f /q "%sdkRoot%\*wsLongPaths.dll" 2>nul
del /s /f /q "%sdkRoot%\*wow64helper.exe" 2>nul

IF NOT defined buildToolsRevision SET buildToolsRevision=28.0.3

rem the aidl.exe arch is x86 and not support longpath
rem modify or add build-tool revisions which you want use in your android project
call :InstPatch "%sdkRoot%\build-tools\%buildToolsRevision%" aidl.exe
call :InstPatch "%sdkRoot%\cmake\3.6.4111459\bin" ninja.exe
call :InstPatch "%sdkRoot%\cmake\3.6.4111459\bin" cmake.exe

goto :L_exit

rem ----------------------- subprocedure -------------------------------------

:InstPatch
set instDir=%1
set instApp=%2
set rediretApp=%3
echo Installing patch for %instApp%...

if defined rediretApp wsls-copy %rediretApp% "%instDir%\%rediretApp%"

fc wsls-core.exe "%instDir%\%instApp%" 1>nul 2>nul
if not %errorlevel%==0 goto :L_continue

echo The patch for %instApp% already installed.
goto :L_exit

:L_continue
if not exist "%instDir%\ndk-%instApp%" wsls-copy "%instDir%\%instApp%" "%instDir%\ndk-%instApp%"

call wsls-arch.exe "%instDir%\ndk-%instApp%"
set arch=x%errorlevel%
echo "%instDir%\ndk-%instApp%" arch is: %arch%
wsls-copy %arch%\wsls-core.exe "%instDir%\%instApp%"
if exist %arch%\%instApp%.bridge wsls-copy %arch%\%instApp%.bridge "%instDir%\%instApp%.bridge"

echo Installing patch for %instApp% succeed.
goto :eof

:L_exit
ping /n 3 127.0.1 >nul
goto :eof
