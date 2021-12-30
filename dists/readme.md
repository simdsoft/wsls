* Revision: 3.4
* Since v3.4, download ```ndk-wsls-3.x.zip``` from [releases](https://github.com/simdsoft/wsLongPaths/releases) or build all binaries by youself except gnumake.exe with ```vs2019 + cmake``` before run install.bat, see follow build steps:
  1. Ensure vs2019 and cmake-3.10 or later installed
  2. Run follow commands:
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
  3. Now the required binaries are in wsLongPaths/dists/
* install.bat
    ```bat
      install.bat <path\to\ndk\> <path\to\sdk>
      rem if no ndk path specified, will use system var ANDROID_NDK
      rem if no sdk path specified, will use system var ANDROID_SDK
    ```
* Project url: https://github.com/simdsoft/wsLongPaths
* gnumake.exe: build from http://ftp.gnu.org/gnu/make/ 4.3, because ndk-make maybe too old, always said: ```ndk-make: *** INTERNAL: readdir: Invalid argument.  Stop.```
* Requirement: Visual Studio 2019 Redist (x86) and (x64) required, can be download at: https://www.x-studio.net/downloads

* gnumake: 
  * build.bat: can build x64 with vs2019, official build_win32.bat has bug
  * build_x86.bat: can build x86 with vs2019, call build.bat

* android-ndk: should use x64
* android-sdk: will have x86 tool, such as aidl.exe in build-tools/{revision}/aidl.exe also doesn't support Long Path
