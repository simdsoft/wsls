@REM v3.5.2 This script uninstall wsls core binaries
@REM uninstall0.bat
@echo off
:: BatchGotAdmin
::-------------------------------------
REM  --> Check for permissions
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

REM --> If error flag set, we do not have admin.
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
echo UAC.ShellExecute "cmd.exe", "/c %~s0 %*", "", "runas", 1 >> "%temp%\getadmin.vbs"

"%temp%\getadmin.vbs"
del "%temp%\getadmin.vbs"
exit /B

:gotAdmin
pushd %cd%
cd /d "%~dp0"

del /q /f %WINDIR%\System32\wsLongPaths.dll 2>nul
if exist %WINDIR%\SysWow64\ (
  del /q /f %WINDIR%\SysWow64\wsLongPaths.dll 2>nul
  
  del /q /f %WINDIR%\System32\wsls-copy.exe 2>nul
  del /q /f %WINDIR%\System32\wsls-del.exe 2>nul
  del /q /f %WINDIR%\System32\wsls-md.exe 2>nul
  del /q /f %WINDIR%\System32\wow64helper.exe 2>nul
  del /q /f %WINDIR%\System32\wsls-core.dll 2>nul
  
  del /q /f %WINDIR%\SysWow64\wsls-copy.exe 2>nul
  del /q /f %WINDIR%\SysWow64\wsls-del.exe 2>nul
  del /q /f %WINDIR%\SysWow64\wsls-md.exe 2>nul
  del /q /f %WINDIR%\SysWow64\wow64helper.exe 2>nul
  del /q /f %WINDIR%\SysWow64\wsls-core.dll 2>nul
) else (
  del /q /f %WINDIR%\System32\wsls-copy.exe 2>nul
  del /q /f %WINDIR%\System32\wsls-del.exe 2>nul
  del /q /f %WINDIR%\System32\wsls-md.exe 2>nul
  del /q /f %WINDIR%\System32\wow64helper.exe 2>nul
  del /q /f %WINDIR%\System32\wsls-core.dll 2>nul
)

REM exit uninstall0.bat script
:L_exit
echo Done, exiting uninstall0.bat...&ping /n 2 127.0.1 >nul
goto :eof
