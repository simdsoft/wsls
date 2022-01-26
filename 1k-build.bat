@echo off

rem Build x86 binaries, android sdk require x86 patch binaries
cmake -B build_x86 -A Win32
cmake --build build_x86 --config Release --target INSTALL

rem Build x64 binaries, android-ndk x64
cmake -B build_x64
cmake --build build_x64 --config Release --target INSTALL
