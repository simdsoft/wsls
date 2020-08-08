# wsLongPaths

[![Release](https://img.shields.io/badge/release-v3.3-blue.svg)](https://github.com/simdsoft/wsLongPaths/releases)

* Purpose: This patch should make any x86/x64 applications support LongPath on windows 7 x64+
* Feature: This patch can make Android ndk r14 or later support LongPaths on windows 7 x64+
* Update since v3.3: Android Studio CMake build system was supported on win10, win7 not test yet
* Remark: Please ensure your anti-virus software allow wsLongPaths.dll & wow64helper.exe

## Core file description
* wsLongPaths.dll: the core file to make any application support Long Paths
* wow64helper.exe: the inject helper for inject wsLongPaths.dll to target application

## Who need this patch?
* You only want windows, and encounter long path issue on exist software system, such as android ndk,sdk
* Even win10 provide LongPath ware system config, but still doesn't works for android build system when path too long
* Some windows systwm command does't support long path even open system LongPath ware config, may windows bug or performance purpose,  
if you find copy, md, del works with long path on future release of win10, please info me.
  
## Install patch for android ndk & sdk:  
1. Set env var ```ANDROID_NDK``` to your ndk-bundle directory  
2. Install ndk: right click ```dists/install.bat```, run as administrator.  
3. After ndk installed, install sdk: ```dists/install-sdk.bat <path\to\sdk> <build-tools-revision>```, such as: ```dists/install-sdk.bat d:\dev\adt\sdk 28.0.3```

## References
* https://github.com/simdsoft/wow64helper
* https://github.com/TsudaKageyu/minhook
* https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
