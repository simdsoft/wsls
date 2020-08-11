* Revision: 3.4
* Since v3.4, you should build all binaries by youself except gnumake.exe with ```cmake + vs2019``` before run install.bat or download ```wsls-3.x.zip``` from [Releases](https://github.com/simdsoft/wsLongPaths/releases)
* install.bat
```bat
  install.bat <path\to\ndk\> <path\to\sdk>
  rem if no ndk path specified, will use system var ANDROID_NDK
  rem if no sdk path specified, will use system var ANDROID_SDK
```
* Project url: https://github.com/simdsoft/wsLongPaths
* gnumake.exe: build from http://ftp.gnu.org/gnu/make/ 4.3, because ndk-make maybe too old, always said: ```ndk-make: *** INTERNAL: readdir: Invalid argument.  Stop.```
* Requirement: Visual Studio 2019 Redist (x86) and (x64) required, can be download at: https://dl.x-studio.net/

* gnumake: 
  * build.bat: can build x64 with vs2019, official build_win32.bat has bug
  * build_x86.bat: can build x86 with vs2019, call build.bat

* android-ndk: should use x64
* android-sdk: will have x86 tool, such as aidl.exe in build-tools/{revision}/aidl.exe also doesn't support Long Path
