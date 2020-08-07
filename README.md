# wsLongPaths

[![Release](https://img.shields.io/badge/release-v3.2-blue.svg)](https://github.com/simdsoft/wsLongPaths/releases)

* Purpose: This is a generic LongPaths patch for any x86/x64 application which does not support LongPaths.
* This patch can make Android ndk r14 or later support LongPaths on windows platform.
  
# wow64helper
https://github.com/simdsoft/wow64helper
  
# minhook
https://github.com/TsudaKageyu/minhook
  
# Microsoft Long Paths Support document
https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
  
  
# Install patch for android ndk:  
1. set env var ```ANDROID_NDK``` to your ndk-bundle directory  
2. right click ```dists/install.bat```, run as administrator.  

# Install patch for android sdk:
1. Install patch for android ndk first
2. ```dists/install-sdk.bat <path\to\sdk> <build-tools-revision>```, such as: ```dists/install-sdk.bat d:\dev\adt\sdk 28.0.3```


