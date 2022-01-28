// wsls-md.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include "libwsls.h"

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
            iRet = wsls::mkdir(std::move(sourcePath));
            if(iRet == 0)
                wprintf(L"wsls-md: %s\n", sourcePath.c_str());
            else
                fwprintf(stderr, L"wsls-md: %s failed, error: %d\n", sourcePath.c_str(), iRet);
        }
    }

    LocalFree(argv);

    return iRet;
}

