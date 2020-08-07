// stdafx.cpp : source file that includes just the standard includes
// libwsls.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
// version: V3.2 2020.8.7 r5
#include <Shlwapi.h>
#include "libwsls.h"
#pragma comment(lib, "Shlwapi.lib")

#define UNC_PREFIX "\\\\?\\"
#define UNC_PREFIXW L"\\\\?\\"

// pitfall: CRT file APIs will may failed with > 248, so we subtract 15 to make CRT happy.
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
        HANDLE fileHandle = ::CreateFileW(wsls::from_chars(fileName).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, nullptr);
        if (fileHandle != INVALID_HANDLE_VALUE) {
            DWORD hi = 0;
            auto n = ::GetFileSize(fileHandle, &hi);
            data.resize(n);
            DWORD sizeRead = 0;
            BOOL succeed = ::ReadFile(fileHandle, &data.front(), n, &sizeRead, nullptr);
            if (!succeed)
                data.resize(0);
            ::CloseHandle(fileHandle);
        }
        return data;
    }

    int writeFileData(const char* fileName, const std::string& content, bool append)
    {
        auto styledPath = makeStyledPath(fileName);
        if (styledPath.empty()) {
            styledPath = wsls::from_chars(fileName);
        }
        HANDLE hFile = CreateFileW(styledPath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            0,
            nullptr);

        if (hFile != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER li;
            ::GetFileSizeEx(hFile, &li);
            auto size = li.QuadPart;
            if (size > 0 && append) {
                li.QuadPart = 0;
                ::SetFilePointerEx(hFile, li, nullptr, FILE_END);
            }
            else {
                ::SetEndOfFile(hFile);
            }

            DWORD bytesToWrite = 0;
            WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.size()), &bytesToWrite, nullptr);

            CloseHandle(hFile);
            return 0;
        }

        return GetLastError();
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

    const char* getFileShortName(std::string_view _FileName)
    {
        auto slash = _FileName.find_last_of("/\\");
        if (slash != std::wstring::npos)
            return _FileName.data() + slash + 1;
        return !_FileName.empty() ? _FileName.data() : "";
    }

    static  std::wstring makeStyledPathInternal(bool uncPrefix, const wchar_t* _FileName)
    { /*
      clang 5.0.2
      android-ndk-r16b clang 5.0.300080
      will simply add prefix R"(\\?\)" for too long path,
      but it's not ok, we should convert it to windows styled path for windows File APIs happy.
      */
        if (!uncPrefix || !isStyledWindowsPath(_FileName + UNC_PREFIX_LEN)) {
            size_t offset = UNC_PREFIX_LEN;
            if (uncPrefix) offset = 0;

            int nBufferLength = GetFullPathNameW(_FileName, 0, nullptr, nullptr);
            if (nBufferLength > 0) {
                std::wstring uncPath(nBufferLength + offset, '\0');
                memcpy(&uncPath.front(), UNC_PREFIXW, offset << 1);

                if (GetFullPathNameW(_FileName, nBufferLength, &uncPath.front() + offset, nullptr) < nBufferLength)
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

    std::wstring makeStyledPath(const char* _FileName) 
    {
        bool uncPrefix = hasUNCPrefix(_FileName);
        if ((_FileName != nullptr && strlen(_FileName) > LONG_PATH_THRESHOLD)
            || uncPrefix) // If already prefix, we need to fix path to styled windows path.
        {
            auto wFileName = wsls::from_chars(_FileName);
            return makeStyledPathInternal(uncPrefix, wFileName.c_str());
        }

        return L"";
    }

    std::wstring makeStyledPath(const wchar_t* _FileName)
    {
        bool uncPrefix = hasUNCPrefix(_FileName);
        if ((_FileName != nullptr && wcslen(_FileName) > LONG_PATH_THRESHOLD)
            || uncPrefix) // If already prefix, we need to fix path to styled windows path.
        {
            return makeStyledPathInternal(uncPrefix, _FileName);
        }

        return L"";
    }

    template<typename _Elem, typename _Fty> inline
        void splitpath(_Elem* s, const _Fty& op)
    {
        _Elem* _Start = s; // the start of every string
        _Elem* _Ptr = s;   // source string iterator
        if (hasUNCPrefix(_Ptr))
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
        splitpath(&_Path.front(), [&](const wchar_t* subdir) {
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
        auto lpCmdLine = GetCommandLineW();

        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };

        si.cb = sizeof(si);

        GetStartupInfo(&si);

        wchar_t currentDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDir);

        wchar_t filePath[MAX_PATH];
        GetModuleFileName(NULL, filePath, MAX_PATH);

        PathRemoveFileSpec(filePath);

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

        si.lpTitle = &title.front();
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

        if (!succeed) {
            wprintf(L"make bridge failed %s --> %s, create original process failed!", shell, app);
            return GetLastError();
        }

        SHELLEXECUTEINFOW sei = { sizeof(SHELLEXECUTEINFOW) };

        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;

        std::wstring parameter = sfmt(L"1 %u ntdll.dll LdrLoadDll u64;u64;us;s 0 0 wsLongPaths.dll 00000000", pi.dwProcessId/*, fileName*/);

        std::wstring wow64helper = L"wow64helper.exe";

        sei.lpFile = wow64helper.c_str();
        sei.lpParameters = parameter.c_str();
        sei.lpDirectory = filePath;

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
