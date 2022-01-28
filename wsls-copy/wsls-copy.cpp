// wsls-copy.cpp: Defines the entry point for the console application.
// 2.2

#include "stdafx.h"
#include <Windows.h>
#include "../libwsls/libwsls.h"

int main(int /*argc*/, char** /*argv*/)
{
    // MessageBox(nullptr, L"Waiting for debugger to attach...", L"tips", MB_OK | MB_ICONEXCLAMATION);
    auto lpszCmdLine = GetCommandLine();

    int n = 0;
    LPWSTR* argv = CommandLineToArgvW(lpszCmdLine, &n);

    std::wstring sourcePath, destPath;

    for (int i = 1; i < n; ++i) {
        std::wstring uncPath;
        auto arg = argv[i];
        if (*arg == '/') continue; // skip ignored options /a /b /d /v /n /y /-y /z /L
        if (wsls::isAbsolutePath(arg)) {
            uncPath = wsls::makeStyledPath(arg);
            if (uncPath.empty()) uncPath = arg;
        }
        else {
            uncPath = LR"(\\?\)";
            if (wsls::isFileExists(arg)) 
            { // convert the relative path to absolute path
                int nreq = GetFullPathNameW(arg, 0, nullptr, nullptr);
                uncPath.resize(nreq + uncPath.size());
                GetFullPathNameW(arg, nreq, &uncPath.front() + 4, nullptr);
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
        if (CopyFile(sourcePath.c_str(), destPath.c_str(), FALSE)) {
            wprintf(L"wsls-copy: %s --> %s\n", sourcePath.c_str(), destPath.c_str());
        }
        else {
            iRet = GetLastError();
            fwprintf(stderr, L"wsls-copy: %s --> %s failed, error: %d\n", sourcePath.c_str(), destPath.c_str(), iRet);
        }
    }

    LocalFree(argv);

    return iRet;
}

