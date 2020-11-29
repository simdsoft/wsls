Welcome to the wsLongPaths wiki!
## How to build this patch binaries and install patch
1. Ensure vs2019 and cmake-3.10 or later installed
2. than run follow commands:
```bat
git clone https://github.com/simdsoft/wsLongPaths
cd wsLongPaths

rem Build x86 binaries, android sdk require x86 patch binaries
cmake -B build_x86 -A Win32
cmake --build build_x86 --config Release --target INSTALL

rem Build x64 binaries, android-ndk x64
cmake -B build_x64
cmake --build build_x64 --config Release --target INSTALL
```
3. Now the required binaries are in wsLongPaths/dists/, just follow project README.md to install patch
