// hooktest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <string>
#include <string_view>
#include <Windows.h>
#include <Shlwapi.h>
#include <Winternl.h>
#include <io.h>
#include "../libwsls/libwsls.h"
#include "../minhook/include/MinHook.h"

NTSTATUS ZwCreateFile(
    _Out_    PHANDLE            FileHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_     POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_    PIO_STATUS_BLOCK   IoStatusBlock,
    _In_opt_ PLARGE_INTEGER     AllocationSize,
    _In_     ULONG              FileAttributes,
    _In_     ULONG              ShareAccess,
    _In_     ULONG              CreateDisposition,
    _In_     ULONG              CreateOptions,
    _In_opt_ PVOID              EaBuffer,
    _In_     ULONG              EaLength
);

#define ENABLE_MSGBOX_TRACE 0

#if defined(_DEBUG)
#pragma comment(lib, "../lib/Debug/libMinHook.x64.lib")
#else
#pragma comment(lib, "../lib/Release/libMinHook.x64.lib")
#endif

#if defined(_DEBUG)
#pragma comment(lib, "../x64/Debug/libwsls.lib")
#else
#pragma comment(lib, "../x64/Release/libwsls.lib")
#endif


#define DEFINE_FUNCTION_PTR(f) static decltype(&f) f##_imp
#define GET_FUNCTION(h,f) f##_imp = (decltype(f##_imp))GetProcAddress(h, #f)
#define HOOK_FUNCTION(f) MH_CreateHook(f##_imp, f##_hook, (LPVOID*)&f##_imp)
#define HOOK_FUNCTION_SAFE(f) if(f##_imp) MH_CreateHook(f##_imp, f##_hook, (LPVOID*)&f##_imp)

DEFINE_FUNCTION_PTR(ZwCreateFile);
DEFINE_FUNCTION_PTR(NtCreateFile);
DEFINE_FUNCTION_PTR(CreateFileW);
DEFINE_FUNCTION_PTR(CreateFileA);
DEFINE_FUNCTION_PTR(fopen);
DEFINE_FUNCTION_PTR(_access);


DEFINE_FUNCTION_PTR(CreateProcessW);
DEFINE_FUNCTION_PTR(GetFileAttributesExW);
DEFINE_FUNCTION_PTR(GetFileAttributesW);
DEFINE_FUNCTION_PTR(FindFirstFileExW);
DEFINE_FUNCTION_PTR(_stat64);

DEFINE_FUNCTION_PTR(GetFullPathNameW);

DWORD
WINAPI
GetFullPathNameW_hook(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength, return +1) LPWSTR lpBuffer,
    _Outptr_opt_ LPWSTR* lpFilePart
)
{
#if ENABLE_MSGBOX_TRACE
    MessageBox(nullptr, L"Call API: GetFullPathNameA...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif
   return GetFullPathNameW_imp(lpFileName, nBufferLength, lpBuffer, lpFilePart);
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
    return CreateProcessW_imp(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
        bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
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

    return GetFileAttributesExW_imp(lpFileName, fInfoLevelId, lpFileInformation);

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
    return GetFileAttributesW_imp(lpFileName);
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

    return CreateFileW_imp(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

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
    MessageBox(nullptr, L"Call API: CreateFileW...", L"Waiting debugger to attach...", MB_OK | MB_ICONEXCLAMATION);
#endif

    return CreateFileA_imp(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
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
    return FindFirstFileExW_imp(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
}

#define LONG_FILE_PATH R"(D:\develop\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\xxxxxxxxxxxxxxxxxxxxxxxxx1.txt)"
#define LONG_FILE_PATHW LR"(D:\develop\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\123456789\xxxxxxxxxxxxxxxxxxxxxxxx2.txt)"

int main()
{
    HMODULE hModule = GetModuleHandle(L"ntdll.dll");

    hModule = GetModuleHandle(L"KernelBase.dll");
    GET_FUNCTION(hModule, CreateFileW);
    GET_FUNCTION(hModule, CreateFileA);
    GET_FUNCTION(hModule, GetFileAttributesW);
    GET_FUNCTION(hModule, GetFileAttributesExW);
    GET_FUNCTION(hModule, FindFirstFileExW);
    GET_FUNCTION(hModule, CreateProcessW);
    GET_FUNCTION(hModule, GetFullPathNameW);

    auto _findfirst64_imp = &_findfirst64;
    hModule = LoadLibrary(L"msvcrt.dll");
    GET_FUNCTION(hModule, fopen);
    GET_FUNCTION(hModule, _access);
    GET_FUNCTION(hModule, _stat64);
    GET_FUNCTION(hModule, _findfirst64);

    MH_Initialize();

    HOOK_FUNCTION(CreateFileW);
    HOOK_FUNCTION(GetFileAttributesExW);
    HOOK_FUNCTION(GetFileAttributesW);

    HOOK_FUNCTION(FindFirstFileExW);
    HOOK_FUNCTION(CreateProcessW);
    HOOK_FUNCTION(GetFullPathNameW);

    MH_EnableHook(MH_ALL_HOOKS);

    __finddata64_t fd;
    _findfirst64_imp(LONG_FILE_PATH, &fd);

    wchar_t buffer[1024];
    GetFullPathNameW(LONG_FILE_PATHW, 1024, buffer, nullptr);

    auto scriptContent = wsls::readFileData(R"(D:\dev\simdsoft\wsls\dists\install.bat)");
   
    return 0;
}

