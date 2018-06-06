// stdafx.cpp : source file that includes just the standard includes
// libwsls.pch will be the pre-compiled header 
// stdafx.obj will contain the pre-compiled type information

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file V2.0 2018.6.6
#include <Shlwapi.h>
#include "libwsls.h"
#pragma comment(lib, "Shlwapi.lib")

#define UNC_PREFIX "\\\\?\\"
#define UNC_PREFIXW L"\\\\?\\"

// pitfall: crt file APIs will may failed with > 248, so we subtract 15 to make crt happy.
#define LONG_PATH_THRESHOLD (MAX_PATH - 12 - 15) 

static const size_t UNC_PREFIX_LEN = sizeof(UNC_PREFIX) - 1;

namespace wsls {
    std::string sfmt(const char* format, ...)
    {
#define CC_VSNPRINTF_BUFFER_LENGTH 512
        va_list args;
        std::string buffer(CC_VSNPRINTF_BUFFER_LENGTH, '\0');

        va_start(args, format);

        int nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
        va_end(args);

        if (nret >= 0) {
            if ((unsigned int)nret < buffer.length()) {
                buffer.resize(nret);
            }
            else if ((unsigned int)nret > buffer.length()) { // VS2015/2017 or later Visual Studio Version
                buffer.resize(nret);

                va_start(args, format);
                nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
                va_end(args);

                // assert(nret == buffer.length());
            }
            // else equals, do nothing.
        }
        else { // less or equal VS2013 and Unix System glibc implement.
            do {
                buffer.resize(buffer.length() * 3 / 2);

                va_start(args, format);
                nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
                va_end(args);

            } while (nret < 0);

            buffer.resize(nret);
        }

        return buffer;
    }

    /*--- This a C++ universal sprintf in the future.
    **  @pitfall: The behavior of vsnprintf between VS2013 and VS2015/2017 is different
    **      VS2013 or Unix-Like System will return -1 when buffer not enough, but VS2015/2017 will return the actural needed length for buffer at this station
    **      The _vsnprintf behavior is compatible API which always return -1 when buffer isn't enough at VS2013/2015/2017
    **      Yes, The vsnprintf is more efficient implemented by MSVC 19.0 or later, AND it's also standard-compliant, see reference: http://www.cplusplus.com/reference/cstdio/vsnprintf/
    */
    std::wstring sfmt(const wchar_t* format, ...)
    {
#define CC_VSNPRINTF_BUFFER_LENGTH 512
        va_list args;
        std::wstring buffer(CC_VSNPRINTF_BUFFER_LENGTH, '\0');

        va_start(args, format);

        int nret = _vsnwprintf(&buffer.front(), buffer.length() + 1, format, args);
        va_end(args);

        if (nret >= 0) {
            if ((unsigned int)nret < buffer.length()) {
                buffer.resize(nret);
            }
            else if ((unsigned int)nret > buffer.length()) { // VS2015/2017 or later Visual Studio Version
                buffer.resize(nret);

                va_start(args, format);
                nret = _vsnwprintf(&buffer.front(), buffer.length() + 1, format, args);
                va_end(args);

                // assert(nret == buffer.length());
            }
            // else equals, do nothing.
        }
        else { // less or equal VS2013 and Unix System glibc implement.
            do {
                buffer.resize(buffer.length() * 3 / 2);

                va_start(args, format);
                nret = _vsnwprintf(&buffer.front(), buffer.length() + 1, format, args);
                va_end(args);

            } while (nret < 0);

            buffer.resize(nret);
        }

        return buffer;
    }

    bool replace_once(std::string& string, const std::string& replaced_key, const std::string& replacing_key)
    {
        std::string::size_type pos = 0;
        if ((pos = string.find(replaced_key, pos)) != std::string::npos)
        {
            (void)string.replace(pos, replaced_key.length(), replacing_key);
            return true;
        }
        return false;
    }

    bool replace_once(std::wstring& string, const std::wstring& replaced_key, const std::wstring& replacing_key)
    {
        std::wstring::size_type pos = 0;
        if ((pos = string.find(replaced_key, pos)) != std::wstring::npos)
        {
            (void)string.replace(pos, replaced_key.length(), replacing_key);
            return true;
        }
        return false;
    }

    int replace(std::wstring& string, const std::wstring& replaced_key, const std::wstring& replacing_key)
    {
        int count = 0;
        std::wstring::size_type pos = 0;
        while ((pos = string.find(replaced_key, pos)) != std::wstring::npos)
        {
            (void)string.replace(pos, replaced_key.length(), replacing_key);
            pos += replacing_key.length();
            ++count;
        }
        return count;
    }

    static bool do_replace(std::wstring& what, const wchar_t* shell, const wchar_t* app)
    {
        if (!replace(what, shell, app)) {
            auto shellExtension = PathFindExtension(shell);
            auto appExtension = PathFindExtension(app);
            if (shellExtension[0] != '\0' && appExtension[0] != '\0')
            {
                std::wstring wshell(shell, shellExtension - shell);
                if (!replace(what, wshell, app))
                {
                    return false;
                }
            }
        }
        return true;
    }

    std::string readFileData(const char* fileName)
    {
        std::string data;
        auto fp = fopen(fileName, "rb");
        if (fp != nullptr) {
            fseek(fp, 0, SEEK_END);
            int n = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            data.resize(n);
            fread(&data.front(), 1, n, fp);
            fclose(fp);
            return data;
        }
        return data;
    }

    void writeFileData(const char* fileName, const std::string& content)
    {
        auto fp = fopen(fileName, "wb");
        if (fp != nullptr) {
            fwrite(content.c_str(), 1, content.size(), fp);
            fclose(fp);
        }
    }

    void convertPathToWinStyle(std::string& path, size_t offset)
    {
        for (size_t i = offset; i < path.length(); ++i)
            if (path[i] == '/')
                path[i] = '\\';
    }

    void convertPathToWinStyle(std::wstring& path, size_t offset)
    {
        for (size_t i = offset; i < path.length(); ++i)
            if (path[i] == '/')
                path[i] = '\\';
    }

    void convertPathToUnixStyle(std::wstring& path, size_t offset)
    {
        for (size_t i = offset; i < path.length(); ++i)
            if (path[i] == '\\')
                path[i] = '/';
    }

    bool isExecFileExist(const wchar_t* _FileName)
    {
        return true;
    }
    std::wstring transcode$IL(std::string_view mcb, UINT cp)
    {
        int cchWideChar = MultiByteToWideChar(cp, 0, mcb.data(), mcb.length(), NULL, 0);
        std::wstring buffer(cchWideChar, '\0');
        MultiByteToWideChar(cp, 0, mcb.data(), mcb.length(), &buffer.front(), cchWideChar);
        return buffer;
    }

    std::string transcode$IL(std::wstring_view wcb, UINT cp)
    {
        int cchMultiByte = WideCharToMultiByte(cp, 0, wcb.data(), wcb.length(), NULL, 0, NULL, NULL);
        std::string buffer(cchMultiByte, '\0');
        WideCharToMultiByte(cp, 0, wcb.data(), wcb.length(), &buffer.front(), cchMultiByte, NULL, NULL);
        return buffer;
    }

    std::wstring makeStyledPath(const char* _FileName)
    {
        if (_FileName != nullptr && strlen(_FileName) > LONG_PATH_THRESHOLD && !isStyledLongPath(_FileName))
        {
            auto wFileName = transcode$IL(_FileName);
            int nBufferLength = GetFullPathNameW(wFileName.c_str(), 0, nullptr, nullptr);
            if (nBufferLength > 0) {
                std::wstring uncPath(nBufferLength + UNC_PREFIX_LEN, '\0');
                memcpy(&uncPath.front(), UNC_PREFIXW, UNC_PREFIX_LEN << 1);

                if (GetFullPathNameW(wFileName.c_str(), nBufferLength, &uncPath.front() + UNC_PREFIX_LEN, nullptr) < nBufferLength)
                {
#if defined(_DEBUG)
                    _wsystem(sfmt(LR"(echo "wsLongPath.dll: convert NON-UNC long path to UNC Path: %s")", uncPath.c_str()).c_str());
#endif
                    return uncPath;
                }
            }
        }

        return L"";
    }

    bool isFileExists(const wchar_t* _Path)
    {
        auto attr = ::GetFileAttributesW(_Path);
        return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool isDirectoryExists(const wchar_t* _Path)
    {
        auto attr = ::GetFileAttributesW(_Path);
        return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool isAbsolutePath(const wchar_t* strPath)
    {
        return (((strPath[0] >= 'a' && strPath[0] <= 'z') || (strPath[0] >= 'A' && strPath[0] <= 'Z'))
            && strPath[1] == ':');

    }

    std::wstring makeStyledPath(const wchar_t* _FileName)
    {
        if (_FileName != nullptr && wcslen(_FileName) > LONG_PATH_THRESHOLD && !isStyledLongPath(_FileName))
        {
            int nBufferLength = GetFullPathNameW(_FileName, 0, nullptr, nullptr);
            if (nBufferLength > 0) {
                std::wstring uncPath(nBufferLength + UNC_PREFIX_LEN, '\0');
                memcpy(&uncPath.front(), UNC_PREFIXW, UNC_PREFIX_LEN << 1);

                if (GetFullPathNameW(_FileName, nBufferLength, &uncPath.front() + UNC_PREFIX_LEN, nullptr) < nBufferLength)
                {
#if defined(_DEBUG)
                    _wsystem(sfmt(LR"(echo "wsLongPath.dll: convert NON-UNC long path to UNC Path: %s")", uncPath.c_str()).c_str());
#endif
                    return uncPath;
                }
            }
        }

        return L"";
    }

    template<typename _Elem, typename _Fty> inline
        void dir_split(_Elem* s, const _Fty& op)
    {
        _Elem* _Start = s; // the start of every string
        _Elem* _Ptr = s;   // source string iterator
        if (isStyledLongPath(_Ptr))
            _Ptr += (sizeof(UNC_PREFIX) - 1);
        while (*_Ptr != '\0')
        {
            if ('\\' == *_Ptr || '/' == *_Ptr)
            {
                ++_Ptr;
                if (_Ptr != _Start) {
                    auto _Ch = *_Ptr;
                    *_Ptr = '\0';
                    bool should_brk = op(s);
                    *_Ptr = _Ch;
                    if (should_brk) {
                        return;
                    }
                }
                _Start = _Ptr;
            }
            else ++_Ptr;
        }
        if (_Start != _Ptr) {
            op(s);
        }
    }
    int mkdir(std::wstring&& _Path)
    {
        int error = 0;
        dir_split(&_Path.front(), [&](const wchar_t* subdir) {
            if (!isDirectoryExists(subdir))
            {
                if (!::CreateDirectoryW(subdir, nullptr)) {
                    error = ::GetLastError();
                    return true;
                }
            }
            return false;
        });
        return error;
    }

    int make_bridge(const wchar_t* shell, const wchar_t* app)
    {
        //#if defined(_DEBUG) 
        //	MessageBox(nullptr, sfmt(L"%s --> %s", shell, app).c_str(), L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
        //#endif

        auto lpCmdLine = GetCommandLineW();

        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };

        si.cb = sizeof(si);

        GetStartupInfo(&si);

        wchar_t currentDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDir);

        wchar_t fileName[MAX_PATH];
        GetModuleFileName(NULL, fileName, MAX_PATH);

        PathRemoveFileSpec(fileName);

        std::wstring maketool = lpCmdLine;

        int argc = 0;
        auto wargv = CommandLineToArgvW(lpCmdLine, &argc);
        if (wargv == nullptr || argc <= 0) {
            return ERROR_INVALID_PARAMETER;
        }

        if (!isExecFileExist(wargv[0])) {
            wprintf(L"make bridge failed %s --> %s, file not found", shell, app);
            return ERROR_FILE_NOT_FOUND;
        }

        std::wstring param0 = wargv[0];
        if (!do_replace(param0, shell, app))
        {
            wprintf(L"make bridge failed %s --> %s", shell, app);
            return ERROR_OPERATION_ABORTED;
        }

        if (!replace_once(maketool, wargv[0], param0))
        {
            wprintf(L"make bridge failed %s --> %s", shell, app);
            return ERROR_OPERATION_ABORTED;
        }

        LocalFree(wargv);

        std::wstring title = si.lpTitle;
        do_replace(title, shell, app);

        DWORD flags = CREATE_SUSPENDED;
        BOOL succeed = CreateProcessW(nullptr,
            &maketool.front(),
            nullptr,
            nullptr,
            TRUE,
            flags,
            nullptr,
            currentDir,
            &si,
            &pi
        );

        SHELLEXECUTEINFOW sei = { sizeof(SHELLEXECUTEINFOW) };

        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;

        std::wstring parameter = sfmt(L"1 %u ntdll.dll LdrLoadDll u64;u64;us;s 0 0 %s\\wsLongPaths.dll 00000000", pi.dwProcessId, fileName);

        std::wstring wow64helper = sfmt(L"%s\\wow64helper.exe", fileName);

        sei.lpFile = wow64helper.c_str();
        sei.lpParameters = parameter.c_str();
        sei.lpDirectory = fileName;

        succeed = ShellExecuteEx(&sei);

        //@*** Wait inject complete
        WaitForSingleObject(sei.hProcess, INFINITE);
        CloseHandle(sei.hProcess);

        //@*** wait process finished.
        ResumeThread(pi.hThread);
        WaitForSingleObject(pi.hThread, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

#if _DEBUG
        wprintf(L"=============> %s retval=%d\n", app, exitCode);
#endif
        return exitCode;
    }

};
