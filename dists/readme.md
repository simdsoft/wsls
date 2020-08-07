* Revision: 3.2
* Project url: https://github.com/halx99/wsLongPaths
* gnumake.exe: build from http://ftp.gnu.org/gnu/make/ 4.3
* Requirement: Visual Studio 2019 Redist (x86) and (x64) required, can be download at: https://dl.x-studio.net/

* gnumake: 
  * build.bat: can build x64 with vs2019, official build_win32.bat has bug
  * build_x86.bat: can build x86 with vs2019, call build.bat

* android-ndk: should use x64
* android-sdk: will have x86 tool, such as aidl.exe in build-tools/{revision}/aidl.exe also doesn't support Long Path