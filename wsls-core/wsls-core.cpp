// wsls-core.cpp : Defines the exported functions for the DLL application.
// V3.4

#include "stdafx.h"
#include <stdio.h>
#include <string>
#include <string_view>
#include <Windows.h>
#include <Shlwapi.h>
#include <io.h>
#include "libwsls.h"
#include "MinHook.h"

#define ENABLE_MSGBOX_TRACE 0

#define DEBUG_MODULE L"aidl.exe"

#define DEFINE_FUNCTION_PTR(f) static decltype(&f) f##_imp
#define GET_FUNCTION(h,f) f##_imp = (decltype(f##_imp))GetProcAddress(h, #f)
#define HOOK_FUNCTION(f) MH_CreateHook(f##_imp, f##_hook, (LPVOID*)&f##_imp)
#define HOOK_FUNCTION_SAFE(f) if(f##_imp) MH_CreateHook(f##_imp, f##_hook, (LPVOID*)&f##_imp)

// The undocumented API
BOOL
WINAPI
MoveFileWithProgressTransactedW(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID lpData,
    _In_     DWORD dwFlags,
    _In_     LPVOID lpUnknown
);

/*
copy命令, 无论怎样均不支持超过249的长路径
echo命令, 可以支持260长路径, 但必须有UNC前缀, 且路径必须是反斜杠
ndk提供的echo.exe,(可以支持260长路径, 但必须有UNC前缀, 且路径必须是反斜杠)
del命令, 无论如何都不支持260长路径
*/
DEFINE_FUNCTION_PTR(CreateProcessA);
DEFINE_FUNCTION_PTR(CreateProcessW);
DEFINE_FUNCTION_PTR(CreateFileA);
DEFINE_FUNCTION_PTR(CreateFileW);
DEFINE_FUNCTION_PTR(GetFullPathNameA);
DEFINE_FUNCTION_PTR(GetFileAttributesA);
DEFINE_FUNCTION_PTR(GetFileAttributesW);
DEFINE_FUNCTION_PTR(GetFileAttributesExW);
DEFINE_FUNCTION_PTR(FindFirstFileExW);
DEFINE_FUNCTION_PTR(DeleteFileW);

// since v3.3: cmake support
// ndk-build: use 'md' command to create directory
// cmake: use CreateDirectoryW API to create directory
DEFINE_FUNCTION_PTR(CreateDirectoryW);

/// <summary>
/// since v3.3: crt rename --> wrename --> MoveFileExW(KernelBase.dll) --> MoveFileWithProgressW
/// msvcrt --> MoveFileA --> MoveFileWithProgressW
/// </summary>
/// 
DEFINE_FUNCTION_PTR(MoveFileExW);
DEFINE_FUNCTION_PTR(MoveFileWithProgressTransactedW);
DEFINE_FUNCTION_PTR(SetFileInformationByHandle);
/*
* // Renames the file named 'old_name' to be named 'new_name'.  Returns zero if
// successful; returns -1 and sets errno and _doserrno on failure.
extern "C" int __cdecl _wrename(wchar_t const* const old_name, wchar_t const* const new_name)
{
    // The MOVEFILE_COPY_ALLOWED flag alloes moving to a different volume.
    if (!MoveFileExW(old_name, new_name, MOVEFILE_COPY_ALLOWED))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    return 0;
}
*/

static std::wstring s_appName;
static std::wstring s_oringalAppName;

// replaceAsOriginal
static std::wstring replaceAsOriginal(const wchar_t* s)
{
    if (s != nullptr) {
        std::wstring o = s;
        wsls::replace_once(o, s_appName, s_oringalAppName);
        return o;
    }
    return L"";
}

BOOL
WINAPI
CreateProcessA_hook(
    _In_opt_ LPCSTR lpApplicationName,
    _Inout_opt_ LPSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOA lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: CreateProcessA...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif
    if (lpApplicationName != nullptr && stricmp(PathFindExtensionA(lpApplicationName), ".bat") == 0) {
        std::string fileContent = wsls::readFileData(lpApplicationName);

        size_t offset = std::string::npos;
        if (fileContent.find("wsls-echo.exe") == std::string::npos && (offset = fileContent.find("echo.exe")) != std::string::npos)
        {
            fileContent.replace(offset, sizeof("echo.exe") - 1, "wsls-echo.exe");
            auto rpos = fileContent.find(">>", offset + sizeof("wsls-echo.exe") - 1);
            if (rpos != std::string::npos) {
                fileContent.replace(rpos, 2, "-a");
            }
            else {
                rpos = fileContent.find_first_of('>');
                if(rpos != std::string::npos) fileContent.replace(rpos, 1, "-w");
            }

            if (rpos != std::string::npos)
            {
                wsls::convertPathToWinStyle(fileContent, rpos + 2);

                wsls::writeFileData(lpApplicationName, fileContent);
            }
        }
        else if (fileContent.find("wsls-copy") == std::string::npos && wsls::replace_once(fileContent, "copy /b/y", "wsls-copy /b/y"))
        {// Replace the system copy which does not support long path >= 249
            wsls::writeFileData(lpApplicationName, fileContent);
        }
        else if (fileContent.find("wsls-del") == std::string::npos && wsls::replace_once(fileContent, "del /f/q", "wsls-del /f/q"))
        {// Replace the system del which does not support long path >= 249
            wsls::writeFileData(lpApplicationName, fileContent);
        }
        else if (fileContent.find("wsls-md") == std::string::npos && wsls::replace_once(fileContent, R"(md ")", R"(wsls-md ")"))
        {// Replace the system del which does not support long path >= 249, Fix windows 7 md LongPath issue
            wsls::writeFileData(lpApplicationName, fileContent);
        }

#if defined(_DEBUG)
        OutputDebugStringA(wsls::sfmt("Create Process:%s, content:%s\n",
            lpApplicationName,
            fileContent.c_str()
        ).c_str());
#endif
    }
#if defined(_DEBUG)
    else { // execute command line
        OutputDebugStringA(wsls::sfmt("execute command: %s\n", lpCommandLine).c_str());
    }
#endif
    return CreateProcessA_imp(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
        bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

BOOL
WINAPI
CreateProcessW_hook(
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: CreateProcessW...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif
    std::wstring strApplication = replaceAsOriginal(lpApplicationName);
    std::wstring strCmdLine = replaceAsOriginal(lpCommandLine);
    
    return CreateProcessW_imp(!strApplication.empty() ? strApplication.c_str() : nullptr, 
        &strCmdLine.front(), 
        lpProcessAttributes, lpThreadAttributes,
        bInheritHandles, dwCreationFlags, lpEnvironment, 
        lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

HANDLE
WINAPI
CreateFileA_hook(
    _In_ LPCSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: CreateFileA...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif
    auto styledPath = wsls::makeStyledPath(lpFileName);
    if (styledPath.empty())
        return CreateFileA_imp(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

    return CreateFileW_imp(styledPath.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

HANDLE
WINAPI
CreateFileW_hook(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: CreateFileW...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif
    auto styledPath = wsls::makeStyledPath(lpFileName);
    if (styledPath.empty())
        return CreateFileW_imp(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

    return CreateFileW_imp(styledPath.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL
WINAPI
CreateDirectoryW_hook(
    _In_ LPCWSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
) 
{
    auto styledPath = wsls::makeStyledPath(lpPathName);
    if (styledPath.empty())
        return CreateDirectoryW_imp(lpPathName, lpSecurityAttributes);
    return CreateDirectoryW_imp(styledPath.c_str(), lpSecurityAttributes);
}

BOOL
WINAPI
DeleteFileW_hook(
    _In_ LPCWSTR lpFileName
)
{
    auto styledPath = wsls::makeStyledPath(lpFileName);
    return DeleteFileW_imp(styledPath.empty() ? lpFileName : styledPath.c_str());
}

// Old revision of ndk will use msvcrt.dll rename --> call SetFileInformationByHandle to rename file
BOOL
WINAPI
SetFileInformationByHandle_hook(
    _In_ HANDLE hFile,
    _In_ FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
    _In_reads_bytes_(dwBufferSize) LPVOID lpFileInformation,
    _In_ DWORD dwBufferSize
)
{
    if (FileInformationClass == FileRenameInfo) 
    {
        FILE_RENAME_INFO* pInfo = (FILE_RENAME_INFO*)lpFileInformation;
        FILE_RENAME_INFO* pInfoNew = nullptr;
        auto styledPath = wsls::makeStyledPath(pInfo->FileName);
        if (!styledPath.empty()) {
            pInfoNew = (FILE_RENAME_INFO*)malloc(sizeof(FILE_RENAME_INFO) + (styledPath.length() + 1) * 2);
            memcpy(pInfoNew, pInfo, sizeof(FILE_RENAME_INFO));
            pInfoNew->FileNameLength = styledPath.length();
            // lstrcpyW(&pInfoNew->FileName[0], styledPath.c_str());
            memcpy(&pInfoNew->FileName[0], styledPath.c_str(), (styledPath.length() + 1) * 2);
        }

        BOOL bRet = SetFileInformationByHandle_imp(hFile, FileInformationClass, pInfoNew == nullptr ? lpFileInformation : pInfoNew, dwBufferSize);
        if (pInfoNew) free(pInfoNew);
        return bRet;
    }
    
    return SetFileInformationByHandle_imp(hFile, FileInformationClass, lpFileInformation, dwBufferSize);
}

// All of MoveFileA/W, MoveFileExA/W will call MoveFileWithProgressTransactedW
// if MoveFileWithProgressTransactedW not present, make MoveFileExW works, @ndk-r21d
BOOL
WINAPI
MoveFileExW_hook(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_     DWORD    dwFlags
)
{
    auto styledPath = wsls::makeStyledPath(lpExistingFileName);

    auto fileName = PathFindFileNameW(lpNewFileName);
    std::wstring newFilePath;
    if (fileName)
    {
        newFilePath.assign(lpNewFileName, fileName - lpNewFileName);
        auto tmpDir = wsls::makeStyledPath(newFilePath.c_str());
        if (!tmpDir.empty()) newFilePath.swap(tmpDir);
        newFilePath += fileName;
    }

    return MoveFileExW_imp(styledPath.empty() ? lpExistingFileName : styledPath.c_str(),
        newFilePath.empty() ? lpNewFileName : newFilePath.c_str(), dwFlags/* | MOVEFILE_REPLACE_EXISTING*/);
}

BOOL
WINAPI
MoveFileWithProgressTransactedW_hook(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID lpData,
    _In_     DWORD dwFlags,
    _In_     LPVOID pUnknown
)
{
     auto styledPath = wsls::makeStyledPath(lpExistingFileName);
 
     auto fileName = PathFindFileNameW(lpNewFileName);
     std::wstring newFilePath;
     if (fileName)
     {
         newFilePath.assign(lpNewFileName, fileName - lpNewFileName);
         auto tmpDir = wsls::makeStyledPath(newFilePath.c_str());
         if (!tmpDir.empty()) newFilePath.swap(tmpDir);
         newFilePath += fileName;
     }
     
     return MoveFileWithProgressTransactedW_imp(styledPath.empty() ? lpExistingFileName : styledPath.c_str(),
         newFilePath.empty() ? lpNewFileName : newFilePath.c_str(), lpProgressRoutine, lpData, dwFlags/* | MOVEFILE_REPLACE_EXISTING*/, pUnknown);
}

DWORD
WINAPI
GetFullPathNameA_hook(
    _In_ LPCSTR lpFileName,
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength, return +1) LPSTR lpBuffer,
    _Outptr_opt_ LPSTR* lpFilePart
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: GetFullPathNameA...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif

    auto styledPath = wsls::makeStyledPath(lpFileName);
    if (styledPath.empty()) // path could passing to 
        return GetFullPathNameA_imp(lpFileName, nBufferLength, lpBuffer, lpFilePart);

    if (wsls::hasUNCPrefix(styledPath.c_str()))
    { // since v3.2 Long path detected and remove unc prefix don't happy
        styledPath.erase(0, 4); 
    }
    
    // write non unc long path to caller
    int cch = ::WideCharToMultiByte(CP_ACP, 0, styledPath.c_str(), styledPath.length(), NULL, 0, NULL, NULL);
    if (nBufferLength > cch)
        return ::WideCharToMultiByte(CP_ACP, 0, styledPath.c_str(), styledPath.length(), lpBuffer, nBufferLength, NULL, NULL);
    else
        return cch + 1;
}

DWORD
WINAPI
GetFileAttributesW_hook(
    _In_ LPCWSTR lpFileName
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: GetFileAttributesW...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif
    auto styledPath = wsls::makeStyledPath(lpFileName);
    if (styledPath.empty()) {
        return GetFileAttributesW_imp(lpFileName);
    }
    return GetFileAttributesW_imp(styledPath.c_str());
}

BOOL
WINAPI
GetFileAttributesExW_hook(
    _In_ LPCWSTR lpFileName,
    _In_ GET_FILEEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID lpFileInformation
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: GetFileAttributesExW...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif
    auto styledPath = wsls::makeStyledPath(lpFileName);
    if (styledPath.empty())
        return GetFileAttributesExW_imp(lpFileName, fInfoLevelId, lpFileInformation);

    return GetFileAttributesExW_imp(styledPath.c_str(), fInfoLevelId, lpFileInformation);
}

HANDLE
WINAPI
FindFirstFileExW_hook(
    _In_ LPCWSTR lpFileName,
    _In_ FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) LPVOID lpFindFileData,
    _In_ FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_ DWORD dwAdditionalFlags
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: FindFirstFileExW...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif
    auto styledPath = wsls::makeStyledPath(lpFileName);
    if (styledPath.empty())
        return FindFirstFileExW_imp(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);

    return FindFirstFileExW_imp(styledPath.c_str(), fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
}

void InstallHook()
{
    wchar_t appName[MAX_PATH * 2+ 1];
    auto hAppModule = GetModuleHandle(nullptr);
    GetModuleFileNameW(hAppModule, appName, MAX_PATH * 2); // Kernel32.dll --> KernelBase.dll: GetModuleFileNameA --> GetModuleFileNameW 
#if defined(_DEBUG)
    if (wcsstr(appName, DEBUG_MODULE)) {
        MessageBoxW(nullptr, wsls::sfmt(L"Install patch: wsLongPaths.dll for %s succeed.", appName).c_str(), L"Wating for debugger to attaching...", MB_OK | MB_ICONEXCLAMATION);
    }
#endif

#if defined(_DEBUG)
    wchar_t currentDirectory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDirectory);
#endif
    /// --------------------------------------  KernelBase.dll -------------------------------------------------
    /// <summary>
    /// win7/10 follow APIs in KernelBase.dll
    /// crt functions will call KernelBase.dll on win10, so we try hook from KernelBase.dll directly
    /// </summary>
    HMODULE hModule = GetModuleHandle(L"KernelBase.dll");
    GET_FUNCTION(hModule, CreateFileA);

    /* _stat64 --> CreateFileW */
    GET_FUNCTION(hModule, CreateFileW);

    /* because the win32 API GetFullPathNameA can't process long path ware, so we need to patch it to fix */
    GET_FUNCTION(hModule, GetFullPathNameA); 

    /* GetFileAttributesA --> GetFileAttributesW */
    GET_FUNCTION(hModule, GetFileAttributesW); 

    /* GetFileAttributesEx --> GetFileAttributesExW */
    GET_FUNCTION(hModule, GetFileAttributesExW);

    /*  FindFirstFile(A/W), FindFirstFileExA --> FindFirstFileExW */
    GET_FUNCTION(hModule, FindFirstFileExW);

    /* CreateDirectorA(KernelBase.dll) --> CreateDirectoryW(KernelBase.dll) --> NtCreateFile(ntdll.dll) */
    GET_FUNCTION(hModule, CreateDirectoryW);

    /* Kernel32.dll --> DeleteFileA(KernelBase.dll) --> DeleteFileW(KernelBase.dll) --> NtOpenFile(ntdll.dll) */
    GET_FUNCTION(hModule, DeleteFileW);

    /*
    * win10 in Kernel32.dll/KernelBase.dll all MoveFile<Ex>(A/W) depends on MoveFileWithProgressTransactedW
    * win7 in Kernel32.dll, MoveFileA/W, MoveFileExA/W, MoveFile not dependency on MoveFileEx
    */
    GET_FUNCTION(hModule, MoveFileExW);

    /* win10 only, win7 no this API */
    GET_FUNCTION(hModule, MoveFileWithProgressTransactedW);

    /* SetFileInformationByHandle(Kernel32.dll) --> SetFileInformationByHandle(KernelBase.dll) */
    GET_FUNCTION(hModule, SetFileInformationByHandle);

    /// --------------------------------------  Kernel32.dll -------------------------------------------------
    /// The API CreateProcess call flow
    /// Windows 7: kernel32.dll --> ntdll.dll
    /// Windows 10: kernel32.dll --> kernelbase.dll --> ntdll.dll
    hModule = GetModuleHandle(L"Kernel32.dll");
    GET_FUNCTION(hModule, CreateProcessA);
    GET_FUNCTION(hModule, CreateProcessW);

    // win7: Not in KernelBase.dll, Get it from Kernel32.dll again
    if (!MoveFileExW_imp) 
        GET_FUNCTION(hModule, MoveFileExW);

    // TODO?
    /* win7: (CreateDirectorTranscatedA --> CreateDirectoryTransactedW)/CreateDirectoryExA --> CreateDirectoryExW(kernel32.dll) --> NtOpenFile(ntdll.dll) */
    // GET_FUNCTION(hModule, CreateDirectoryExW);

    MH_Initialize();

    HOOK_FUNCTION(CreateProcessA);
    HOOK_FUNCTION(CreateProcessW);
    HOOK_FUNCTION(CreateFileA);
    HOOK_FUNCTION(CreateFileW); 
    HOOK_FUNCTION(GetFullPathNameA);
    HOOK_FUNCTION(GetFileAttributesW);
    HOOK_FUNCTION(GetFileAttributesExW);
    HOOK_FUNCTION(FindFirstFileExW);
    HOOK_FUNCTION(CreateDirectoryW);
    HOOK_FUNCTION(DeleteFileW);
    
    if(MoveFileWithProgressTransactedW_imp)
        HOOK_FUNCTION(MoveFileWithProgressTransactedW);
    else
        HOOK_FUNCTION(MoveFileExW);

    HOOK_FUNCTION(SetFileInformationByHandle);

    MH_EnableHook(MH_ALL_HOOKS);

    s_appName = PathFindFileName(appName);
    if (s_appName.find(L"wrl-") != std::wstring::npos) {
        s_oringalAppName = s_appName.substr(sizeof("wrl-") - 1);
    } 
    else {
        s_oringalAppName = s_appName;
    }
}
