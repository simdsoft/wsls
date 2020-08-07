rem 1. Install android sdk long path patch.
@echo off
cd /d %~dp0

rem currently, the android sdk arch is x86, so set arch to x86
set arch=x86

set sdkRoot=%1
set buildToolsRevision=%2
if not defined sdkRoot set sdkRoot=%ANDROID_SDK%

if not defined sdkRoot echo Please specific ANDROID_SDK! && goto :L_exit

if not exist %sdkRoot% echo The directory not exist! && goto :L_exit

del /s /f /q "%sdkRoot%\*.bridge" 2>nul
del /s /f /q "%sdkRoot%\*wsLongPaths.dll" 2>nul
del /s /f /q "%sdkRoot%\*wow64helper.exe" 2>nul

IF NOT defined buildToolsRevision SET buildToolsRevision=28.0.3

rem the aidl.exe arch is x86 and not support longpath
rem modify or add build-tool revisions which you want use in your android project
call :InstPatch "%sdkRoot%\build-tools\%buildToolsRevision%" aidl.exe

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

fc wsLongPaths.dll "%instDir%\wsLongPaths.dll" 1>nul 2>nul
if not %errorlevel%==0 goto :L_continue

echo The patch for %instApp% already installed.
goto :L_exit

:L_continue
if not exist "%instDir%\sdk-%instApp%" wsls-copy "%instDir%\%instApp%" "%instDir%\sdk-%instApp%"

wsls-copy %arch%\wsls-core.exe "%instDir%\%instApp%"
if exist %arch%\%instApp%.bridge wsls-copy %arch%\%instApp%.bridge "%instDir%\%instApp%.bridge"

rem wsls-copy %arch%\wow64helper.exe "%instDir%\wow64helper.exe"
rem wsls-copy %arch%\wsLongPaths.dll "%instDir%\wsLongPaths.dll"

echo Installing patch for %instApp% succeed.
goto :eof

:L_exit
ping /n 3 127.0.1 >nul
goto :eof
