@echo off
cd /d %~dp0

set ndkRoot=%1

if not defined ndkRoot set ndkRoot=D:\android_devenv\adt-bundle-windows\sdk\ndk-bundle

pushd %ndkRoot%

copy /y prebuilt\windows-x86_64\bin\ndk-make.exe prebuilt\windows-x86_64\bin\make.exe
copy /y prebuilt\windows-x86_64\bin\ndk-cmp.exe prebuilt\windows-x86_64\bin\cmp.exe

lcopy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\ndk-arm-linux-androideabi-g++.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\arm-linux-androideabi-g++.exe
copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\ndk-arm-linux-androideabi-gcc.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\arm-linux-androideabi-gcc.exe

copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ndk-ld.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ld.exe
copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ndk-as.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\as.exe
copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ndk-ar.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin\ar.exe

copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\ndk-cc1.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\cc1.exe
copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\ndk-cc1plus.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\cc1plus.exe
copy /y toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\ndk-collect2.exe toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\libexec\gcc\arm-linux-androideabi\4.9.x\collect2.exe

popd

ping /n 3 127.0.1 >nul
goto :eof
