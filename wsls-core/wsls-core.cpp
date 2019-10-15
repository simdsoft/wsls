// wsls-core.cpp : Defines the entry point for the console application.

#include "stdafx.h"

#include <Windows.h>
#include <Shlwapi.h>
#include "../libwsls/libwsls.h"

#if defined(_DEBUG)
#pragma comment(lib, "../x64/Debug/libwsls.lib")
#else
#pragma comment(lib, "../x64/Release/libwsls.lib")
#endif

int main(int /*argc*/, char** /*argv*/)
{
    char fileName[4096];
    auto n = GetModuleFileNameA(GetModuleHandle(nullptr), fileName, sizeof(fileName));

    if (n > 0) {
        if (stricmp(PathFindFileNameA(fileName), "wsls-core.exe") == 0) {
            MessageBoxA(nullptr, "Please rename wsls-core.exe to what(such as make.exe, cc1plus.exe, arm-linux-androideabi-g++.exe) do you want to hook!", "tips", MB_OK | MB_ICONEXCLAMATION);
            return ERROR_OPERATION_ABORTED;
        }

        std::wstring shell = wsls::transcode$IL(fileName);

        strcpy(fileName + n, ".bridge");
        std::wstring app = wsls::transcode$IL(wsls::readFileData(fileName));
        if (app.empty()) // default is the real compile program file name is prefix 'ndk-' 
            app = L"ndk-" + shell;

        if (!shell.empty() && !app.empty())
            return wsls::make_bridge(shell.c_str(), app.c_str());
        else
            return ERROR_INVALID_PARAMETER;
    }

    return ERROR_UNKNOWN_FEATURE;
}
