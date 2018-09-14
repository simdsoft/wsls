// wsls-echo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <string>
#include "../libwsls/libwsls.h"

#if defined(_DEBUG)
#pragma comment(lib, "../x64/Debug/libwsls.lib")
#else
#pragma comment(lib, "../x64/Release/libwsls.lib")
#endif

int main(int argc, char** argv)
{
    int iRet = 0;

    // MessageBoxA(nullptr, "Waiting for debugger to attach...", "tips", MB_OK | MB_ICONEXCLAMATION);
    bool bNoCRLF = false;
    int redirectType = 0;
    std::string content;
    std::string redirectPath;

    for (auto i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-n") == 0)
        {
            bNoCRLF = true;
        }
        else if (strcmp(argv[i], "-w") == 0)
        {
            redirectType = 1; // overrite
        }
        else if(strcmp(argv[i], "-a") == 0) {
            redirectType = 2; // append
        }
        else { // contents
            auto value = argv[i];
            auto len = strlen(value);

            if (redirectType == 0) { // regard normal content to redirect
                if (!content.empty()) content.push_back(' ');
                content.append(value, len);
            }
            else { // regard as path
                redirectPath.append(value, len);
                break;
            }
        }
    }

    if (!bNoCRLF)
        content.append("\r\n");

    if (redirectType > 0) {
        if (!redirectPath.empty()) {

            iRet = wsls::writeFileData(redirectPath.c_str(), content, redirectType == 2);
            if (iRet == ERROR_PATH_NOT_FOUND) { 
			/* Some times, the ndk system call wsls-md to make directory will failed random.
			It's so strangely, so we do a workaround for redirect command.
			*/
                auto pathend = redirectPath.find_last_of("/\\");
                if (pathend != std::string::npos) {
                    auto endCh = redirectPath[pathend];
                    redirectPath[pathend] = '\0';

                    auto styledPath = wsls::makeStyledPath(redirectPath.c_str());
                    if (styledPath.empty()) styledPath = wsls::transcode$IL(redirectPath.c_str());

                    iRet = wsls::mkdir(std::move(styledPath));
                    if (iRet == 0)
                    {
                        redirectPath[pathend] = endCh;
                        // Try redirect again
                        iRet = wsls::writeFileData(redirectPath.c_str(), content, redirectType == 2);
                    }
                }
                else {
                    iRet = ERROR_INVALID_PARAMETER;
                }
            }
        }
        else {
            iRet = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        fprintf(stdout, "%s", content.c_str());
    }

    return iRet;
}

