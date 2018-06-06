// wsls-copy.cpp : Defines the entry point for the console application.
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
    // MessageBox(nullptr, L"Waiting for debugger to attach...", L"tips", MB_OK | MB_ICONEXCLAMATION);
    auto lpszCmdLine = GetCommandLine();

    int n = 0;
    LPWSTR* argv = CommandLineToArgvW(lpszCmdLine, &n);

    std::wstring sourcePath, destPath;


    for (int i = 1; i < n; ++i) {
        std::wstring uncPath = L"\\\\?\\";

        if (wsls::isAbsolutePath(argv[i]))
            uncPath.append(argv[i]);
        else {
            if (wsls::isFileExists(argv[i]) || !sourcePath.empty()) {
                int nreq = GetFullPathNameW(argv[i], 0, nullptr, nullptr);
                uncPath.resize(nreq + uncPath.size());
                GetFullPathNameW(argv[i], nreq, &uncPath.front() + 4, nullptr);
            }
        }

        if (sourcePath.empty()) {
            if (wsls::isFileExists(uncPath.c_str()))
            {
                sourcePath = std::move(uncPath);
            }
        }
        else {
            if (wsls::isDirectoryExists(uncPath.c_str()))
            {
                if (*uncPath.rbegin() != '\\') {
                    uncPath.push_back('\\');
                }
                else if (*uncPath.rbegin() == '/')
                {
                    *uncPath.rbegin() = '\\';
                }

                auto pos = sourcePath.find_last_of(L"\\/", std::wstring::npos);
                auto sourceFileName = (&sourcePath[pos] + 1);
                uncPath.append(sourceFileName);
            }

            destPath = std::move(uncPath);

            break;
        }
    }

    int iRet = ERROR_INVALID_PARAMETER;
    if (!sourcePath.empty() && !destPath.empty())
    {
        wprintf(L"wsls-copy: %s --> %s", sourcePath.c_str(), destPath.c_str());
        if (CopyFile(sourcePath.c_str(), destPath.c_str(), FALSE)) {
            iRet = 0;
            wprintf(L" succeed.\n");
        }
        else {
            iRet = GetLastError();
            wprintf(L" failed, error: %d\n", iRet);
        }
    }

    LocalFree(argv);

    return iRet;
}

