// wsls-copy.cpp : Defines the entry point for the console application.
//

#include <Windows.h>
#include "libwsls.h"

int main(int /*argc*/, char** /*argv*/)
{
    auto lpszCmdLine = GetCommandLine();

    int n = 0;
    LPWSTR* argv = CommandLineToArgvW(lpszCmdLine, &n);

    std::wstring sourcePath;


    for (int i = 1; i < n; ++i) {
        std::wstring uncPath;

        if (nullptr == wcschr(argv[i], '/')) {
            if (wsls::isAbsolutePath(argv[i])) {
                uncPath = wsls::makeStyledPath(argv[i]);
                if (uncPath.empty()) uncPath = argv[i];
            }
            else if (wsls::isFileExists(argv[i])) {
                uncPath = L"\\\\?\\";
                int nreq = GetFullPathNameW(argv[i], 0, nullptr, nullptr);
                uncPath.resize(nreq + uncPath.size());
                GetFullPathNameW(argv[i], nreq, &uncPath.front() + 4, nullptr);
            }
        }

        if (sourcePath.empty()) {
            if (wsls::isFileExists(uncPath.c_str()))
            {
                sourcePath = std::move(uncPath);
                break;
            }
        }
    }

    int iRet = 0; // ERROR_INVALID_PARAMETER;
    if (!sourcePath.empty())
    {
        if (DeleteFileW(sourcePath.c_str())) {
            wprintf(L"wsls-del: %s\n", sourcePath.c_str());
        }
        else {
            iRet = GetLastError();
            fwprintf(stderr, L"wsls-del: %s failed, error: %d\n", sourcePath.c_str(), iRet);
        }
    }

    LocalFree(argv);

    return iRet;
}

