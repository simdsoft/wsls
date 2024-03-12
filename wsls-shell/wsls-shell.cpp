// wsls-core.cpp : Defines the entry point for the console application.

#include "stdafx.h"

#include <Windows.h>
#include <Shlwapi.h>
#include "libwsls.h"

#if defined(_DEBUG)
#define WSLS_WAIT_DEBUGGER 0
#else
#define WSLS_WAIT_DEBUGGER 0
#endif

int main(int /*argc*/, char** /*argv*/)
{
    char fileName[4096];
    auto n = GetModuleFileNameA(GetModuleHandle(nullptr), fileName, sizeof(fileName));

    if (n > 0) {
        if (stricmp(PathFindFileNameA(fileName), "wsls-shell.exe") == 0) {
            MessageBoxA(nullptr, "Please rename wsls-shell.exe to what(such as make.exe, cc1plus.exe, arm-linux-androideabi-g++.exe) do you want to bridge!", "tips", MB_OK | MB_ICONEXCLAMATION);
            return ERROR_OPERATION_ABORTED;
        }

        std::wstring shell = wsls::from_chars(wsls::getFileShortName(fileName));

        strcpy(fileName + n, ".bridge");
        std::wstring app = wsls::from_chars(wsls::readFileData(fileName));
        if (app.empty()) // default is the real compile program file name is prefix 'wrl-' 
            app = L"wrl-" + shell;

#if WSLS_WAIT_DEBUGGER 
        MessageBoxW(nullptr, wsls::sfmt(L"%s --> %s", shell.c_str(), app.c_str()).c_str(), L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif

        char* psz_wsls_dist = getenv("wsls_dist");
        char* psz_path = getenv("path");
        std::string path{ psz_path ? psz_path : "" };
        if (path.find(psz_wsls_dist) == std::string::npos) {
            if (psz_wsls_dist) {
                std::string_view wsls_dist{ psz_wsls_dist };
                path.insert(0, wsls_dist);
                if constexpr (sizeof(void*) == 8)
                    path.insert(wsls_dist.size(), wsls_dist.back() == '\\' ? "x64;" : "\\x64;");
                else
                    path.insert(wsls_dist.size(), wsls_dist.back() == '\\' ? "x86;" : "\\x86;");

                SetEnvironmentVariableA("path", path.c_str());
                _putenv_s("path", path.c_str());
                path = getenv("path");
            }
            else {
                fprintf(stderr, "Missing env var 'wsls_dist', please specify it to wsls_dist root directory\n");
                return ERROR_INVALID_ENVIRONMENT;
            }
        }

        if (!shell.empty() && !app.empty())
            return wsls::make_bridge(shell.c_str(), app.c_str());
        else
            return ERROR_INVALID_PARAMETER;
    }

    return ERROR_UNKNOWN_FEATURE;
}
