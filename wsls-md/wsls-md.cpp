// wsls-md.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include "../libwsls/libwsls.h"

#if defined(_DEBUG)
#pragma comment(lib, "../x64/Debug/libwsls.lib")
#else
#pragma comment(lib, "../x64/Release/libwsls.lib")
#endif
int main(int /*argc*/, char** /*argv*/)
{
    auto lpszCmdLine = GetCommandLine();

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(lpszCmdLine, &argc);
    int iRet = ERROR_INVALID_PARAMETER;
    if (argc >= 2) {
        std::wstring sourcePath;

        if (!wsls::hasUNCPrefix(argv[1]))
            sourcePath.append(LR"(\\?\)");
        sourcePath.append(argv[1]);

        if (!sourcePath.empty())
        {
            wprintf(L"wsls-md: %s", sourcePath.c_str());
            iRet = wsls::mkdir(std::move(sourcePath));
        }
    }

    LocalFree(argv);

    return iRet;
}

