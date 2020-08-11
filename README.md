# wsLongPaths

[![Windows Build Status](https://ci.appveyor.com/api/projects/status/nko8ulow8tcftuvp/branch/master?svg=true)](https://ci.appveyor.com/project/halx99/wslongpaths)
[![Release](https://img.shields.io/badge/release-v3.4-blue.svg)](https://github.com/simdsoft/wsLongPaths/releases)

* Purpose: This patch should make any x86/x64 applications support LongPath on win7/win10
* Feature: This patch can make Android ndk r14 or later support LongPaths on win7/win10 x64
* Update since v3.4: Android ndk(r14b~r21) ndk-build/cmake works on win7/win10 x64
* Remark: Please ensure your anti-virus software allow wsls-core.dll & wow64helper.exe

## Core files description
* wsls-core.dll: the core file to make any application support Long Paths
* wow64helper.exe: the inject helper for inject wsls-core.dll to target application

## Who need this patch?
* You only want windows, and encounter long path issue on exist software system, such as android ndk,sdk
* Even win10 provide LongPath ware system config, but still doesn't works for android build system when path too long
* Some windows systwm command does't support long path even open system LongPath ware config, may windows bug or performance purpose,  
if you find copy, md, del works with long path on future release of win10, please info me.
  
## Install patch for android ndk & sdk:  
1. Set env var ```ANDROID_NDK``` to your android ndk root directory  
2. Set env var ```ANDROID_SDK``` to your android sdk root directory  
3. Install ndk: right click ```dists/install.bat```, run as administrator.  

## References
* https://github.com/simdsoft/wow64helper
* https://github.com/TsudaKageyu/minhook
* https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
