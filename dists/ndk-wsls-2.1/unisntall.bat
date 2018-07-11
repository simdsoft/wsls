@echo off
cd /d %~dp0

set ndkRoot=%1
if not defined ndkRoot set ndkRoot=%ANDROID_NDK_ROOT%

if not defined ndkRoot echo Please specific ANDROID_NDK_ROOT! && goto :L_exit

if not exist %ndkRoot% echo The directory not exist! && goto :L_exit

pushd %ndkRoot%

wsls-copy /y prebuilt\windows-x86_64\bin\ndk-make.exe prebuilt\windows-x86_64\bin\make.exe
wsls-copy /y prebuilt\windows-x86_64\bin\ndk-cmp.exe prebuilt\windows-x86_64\bin\cmp.exe

wsls-copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\ndk-arm-linux-androideabi-g++.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\arm-linux-androideabi-g++.exe
wsls-copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\ndk-arm-linux-androideabi-gcc.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\arm-linux-androideabi-gcc.exe

wsls-copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ndk-ld.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ld.exe
wsls-copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ndk-as.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\as.exe
wsls-copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ndk-ar.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ar.exe

wsls-copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\ndk-cc1.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\cc1.exe
wsls-copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\ndk-cc1plus.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\cc1plus.exe
wsls-copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\ndk-collect2.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\collect2.exe

popd

ping /n 3 127.0.1 >nul
goto :eof
