// wsls-core.cpp : Defines the entry point for the console application.

#include "stdafx.h"

#include <Windows.h>
#include <Shlwapi.h>
#include "libwsls.h"

#if defined(_DEBUG)
#define WSLS_WAIT_DEBUGGER 1
#else
#define WSLS_WAIT_DEBUGGER 0
#endif

int main(int argc, char** argv)
{
    static char my_path[WSLS_MAX_PATH];
    auto n = GetModuleFileNameA(GetModuleHandle(nullptr), my_path, sizeof(my_path));

    if (n > 0) {
        std::wstring shell = wsls::from_chars(wsls::getFileShortName(my_path));
        std::wstring app;

        bool forward_mode = stricmp(PathFindFileNameA(my_path), "wsls-shell.exe") == 0;
        if (forward_mode)
        { // execute without replacement
            if (argc < 2) {
                fprintf(stderr, "Missing parameter not exist when runs wsls-shell in as-is mode\nUsage: wsls-shell.exe path/to/app.exe [args...]");
                return ERROR_INVALID_PARAMETER;
            }

            app = ntcvt::from_chars(argv[1]);
            if (!wsls::isFileExists(app.c_str())) {
                fprintf(stderr, "The app %s not exist when runs wsls-shell in as-is mode\n", argv[1]);
                return ERROR_NOT_FOUND;
            }
        }
        else {
            strcpy(my_path + n, ".bridge");
            app = wsls::from_chars(wsls::readFileData(my_path));
            if (app.empty()) // default is the real compile program file name is prefix 'wrl-' 
                app = L"wrl-" + shell;
        }

#if WSLS_WAIT_DEBUGGER 
        MessageBoxW(nullptr, wsls::sfmt(L"%s --> %s", shell.c_str(), app.c_str()).c_str(), L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif

        // setup env
        char* psz_path = getenv("path");
        std::string path{ psz_path ? psz_path : "" };

        std::string wsls_bin;
        char* psz_wsls_dist = getenv("wsls_dist");
        if (!psz_wsls_dist) {
            PathRemoveFileSpecA(my_path);
            wsls_bin = my_path;
        }
        else {
            wsls_bin = psz_wsls_dist;
            if constexpr (sizeof(void*) == 8)
                wsls_bin.append(wsls_bin.back() == '\\' ? "x64" : "\\x64");
            else
                wsls_bin.append(wsls_bin.back() == '\\' ? "x86" : "\\x86");
        }

        if (path.find(wsls_bin) == std::string::npos) {
            path.insert(0, wsls_bin);
            path.insert(wsls_bin.size(), 1, ';');
            SetEnvironmentVariableA("path", path.c_str());
            _putenv_s("path", path.c_str());
            path = getenv("path");
        }

        // execute
        if (!shell.empty() && !app.empty())
            return wsls::make_bridge(shell.c_str(), app.c_str(), forward_mode);
        else
            return ERROR_INVALID_PARAMETER;
    }

    return GetLastError();
}
