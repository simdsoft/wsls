// stdafx.cpp : source file that includes just the standard includes
// wsLongPaths.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h" 

#if defined(_DEBUG)
#pragma comment(lib, "../x64/Debug/libwsls.lib")
#else
#pragma comment(lib, "../x64/Release/libwsls.lib")
#endif

