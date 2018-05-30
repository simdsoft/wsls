#include <Windows.h>
#include <Shlwapi.h>
#include <ShellAPI.h>
#include <stdint.h>
#include <vector>
#include <assert.h>

#define _WAITING_DEBUGGER_ATTACH 0

#define _ENABLE_INJECT_DEBUG 0

#define sz_align(d,a) (((d) + ((a) - 1)) & ~((a) - 1))  
#define calc_stack_size(n) sz_align((n) * 8, 16) + 8

enum OptionFlag
{
    OptionFlagNull, // Use CreateRemoteThread
    OptionFlagRaw = 1, // Use RtlCreateUserThread/RtlExitUserThread
    OptionFlagReturnBool = 2, // Convert Result as Bool value
};

typedef int(*RtlCreateUserThreadProc)(
    void*,					// ProcessHandle
    void*,                  // SecurityDescriptor
    DWORD64,                // CreateSuspended
    DWORD64,                // StackZeroBits
    void*,                  // StackReserved
    void*,                  // StackCommit
    void*,		            // StartAddress
    void* StartParameter,   // StartParameter
    PHANDLE,                // ThreadHandle
    void* lpClientID    // ClientID)
    );

typedef DWORD(*ZwSuspendProcessProc)(HANDLE);
typedef DWORD(*ZwResumeProcessProc)(HANDLE);


template <class T>
struct _UNICODE_STRING_T
{
    union
    {
        struct
        {
            WORD Length;
            WORD MaximumLength;
        };
        T dummy;
    };
    T Buffer;
};

typedef _UNICODE_STRING_T<DWORD64> UNICODE_STRING;
typedef UNICODE_STRING* PUNICODE_STRING;

struct RemoteArg
{
    int type = 0; // 0: integer, 1: string, 2: wstring, 3: UNICODE_STRING
    union {
        char* valuestring;
        wchar_t* valuewstring;
        uint64_t valueint = 0;
        void* ptr;
        UNICODE_STRING* valueus;
    } value;

    void* appendBuffer = nullptr; // FOR UNICODE_STRING buffer
};

inline
char* transcode(const wchar_t* source)
{
    // ASSERT(source != nullptr);
    auto size = WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL);
    char* result = (char*)calloc(1, size);
    WideCharToMultiByte(CP_ACP, 0, source, -1, result, size, NULL, NULL);

    return result;
}

template<typename _Elem, typename _Fty> inline
void split(_Elem* s, const _Elem delim, const _Fty& op)
{
    _Elem* _Start = s; // the start of every string
    _Elem* _Ptr = s;   // source string iterator
    while (*_Ptr != '\0')
    {
        if (delim == *_Ptr/* && _Ptr != _Start*/)
        {
            if (_Ptr != _Start)
                if (op(_Start, _Ptr))
                    break;
            _Start = _Ptr + 1;
        }
        ++_Ptr;
    }
    if (_Start != _Ptr) {
        op(_Start, _Ptr);
    }
}

inline
void* rpalloc(HANDLE hProcess, size_t size, DWORD flProtect = PAGE_READWRITE)
{
    return VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT, flProtect);
}

inline
void rpfree(HANDLE hProcess, void* p)
{
    VirtualFreeEx(hProcess, p, 0, MEM_RELEASE);
}

inline
BOOL rpwrite(HANDLE hProcess, void*p, const void* data, size_t size)
{
    return WriteProcessMemory(hProcess, p, data, size, NULL);
}

inline
void* rpmemdup(HANDLE hProcess, const void* data, size_t size, DWORD fProtect = PAGE_READWRITE)
{
    auto vmem = rpalloc(hProcess, size, fProtect);
    rpwrite(hProcess, vmem, data, size);
    return vmem;
}

char* rpstrdup(HANDLE hProcess, const char* str)
{
    return (char*)rpmemdup(hProcess, str, strlen(str) + 1);
}

wchar_t* rpwcsdup(HANDLE hProcess, const wchar_t* str)
{
    return (wchar_t*)rpmemdup(hProcess, str, (wcslen(str) + 1) << 1);
}

/*

usage:
      wow64helper.exe 0 PID OSModuleName ModuleProcName paramsTypes [parameters]...
      wow64helper.exe 1 PID OSModuleName ModuleProcName paramsTypes [parameters]...

 #currently supported parameter types
     v; [void]
     u64; [uint64_t]
     s; [string]
     ws: [wstring]
     us; [UNICODE_STRING]
 #plan support types:
     u8,u16,u32

 @remark: all int types use reinterpret_cast, unsigned store also support signed int
 wow64helper.exe 1 PID OSModuleName ModuleProcName paramsTypes [parameters]...

example:
         WowRemoteExecuteProc64(1 string arg)         ---> wow64helper.exe 1 2332 GetModuleHandleW "some.x64.dll"
         WowRemoteProc64(1 integer arg)               ---> wow64helper.exe 2 2332 FreeLibrary 7393439
         WowRemoteExecuteKernelProc64(2 integer arg)  ---> wow64helper.exe 3 2332 FreeLibraryAndExitThread 7393439 0
         WowRemoteInject64                            ---> wow64helper.exe 4 2332

*/

/*
 option: 0: use kernel32 CreateRemoteThread
         1: use ntdll RtlCreateUserThread RtlExitUserThread
*/
bool WowExecuteRemoteProc64(int option, HANDLE hProcess, wchar_t* lpModuleName, char* lpProcName, wchar_t* argt, wchar_t** argv, int argc, DWORD& exitCode);

RtlCreateUserThreadProc RtlCreateUserThread;
void*                   RtlExitUserThread;
ZwSuspendProcessProc    ZwSuspendProcess;
ZwResumeProcessProc     ZwResumeProcess;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
#if _WAITING_DEBUGGER_ATTACH
    MessageBox(nullptr, L"waiting for debugger to attach!", L"Tips", MB_OK);
#endif

    int ret = -1;

    auto ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll != nullptr) {
        RtlCreateUserThread = (RtlCreateUserThreadProc)GetProcAddress(ntdll, "RtlCreateUserThread");
        RtlExitUserThread = GetProcAddress(ntdll, "RtlExitUserThread");
		ZwSuspendProcess = (ZwSuspendProcessProc)GetProcAddress(ntdll, "ZwSuspendProcess");
		ZwResumeProcess = (ZwResumeProcessProc)GetProcAddress(ntdll, "ZwResumeProcess");
    }

    auto szCmdLine = GetCommandLine();

    int argc = 0;
    auto argv = CommandLineToArgvW(szCmdLine, &argc);

    if (argc >= 6) {
        int option = wcstol(argv[1], nullptr, 10);

        DWORD dwPID = wcstoul(argv[2], nullptr, 10);
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
		
        if (hProcess == nullptr)
            return ret;

		ZwSuspendProcess(hProcess);

        DWORD exitCode = 0;

        /*
        wow64helper.exe 0 PID OSModuleName ModuleProcName paramsTypes [parameters]...
        wow64helper.exe 1 PID OSModuleName ModuleProcName paramsTypes [parameters]...
        */
        auto lpProcName = transcode(argv[4]);
        bool rpcOK = WowExecuteRemoteProc64(option, hProcess, argv[3] /*moduleName*/, lpProcName/*lpProcName*/, argv[5], argv + 6, argc - 6, exitCode);
        free(lpProcName);

        if (rpcOK)
            ret = static_cast<int>(exitCode);

        CloseHandle(hProcess);
    }

    LocalFree(argv);
    return ret;
}

bool rpcall(int option, HANDLE hProcess, void* StartAddress, void* StartParameter, DWORD& exitCode)
{
    if (option == 0) {
        DWORD threadId;
        HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0,
            (LPTHREAD_START_ROUTINE)StartAddress, StartParameter, 0, &threadId);

        exitCode = 0;

		ZwResumeProcess(hProcess);
        if (hRemoteThread != NULL)
        {
            WaitForSingleObject(hRemoteThread, INFINITE);
            GetExitCodeThread(hRemoteThread, &exitCode);

            CloseHandle(hRemoteThread);
            return true;
        }
        else
        {
            return false;
        }
    }
    else {
        struct {
            DWORD64 UniqueProcess;
            DWORD64 UniqueThread;
        } clientId;

        HANDLE hRemoteThread = INVALID_HANDLE_VALUE;
        int ret = RtlCreateUserThread(
            hProcess,				 // ProcessHandle
            nullptr,                 // SecurityDescriptor
            (DWORD64)FALSE,          // CreateSuspended
            (DWORD64)0,              // StackZeroBits
            nullptr,                 // StackReserved
            nullptr,                 // StackCommit
            StartAddress,	         // StartAddress
            StartParameter,          // StartParameter
            &hRemoteThread,          // ThreadHandle
            &clientId);              // ClientID)

		ZwResumeProcess(hProcess);
        if (INVALID_HANDLE_VALUE != hRemoteThread)
        {
            WaitForSingleObject(hRemoteThread, INFINITE);
            GetExitCodeThread(hRemoteThread, &exitCode);
            CloseHandle(hRemoteThread);
            return true;
        }
        else {
            return false;
        }
    }
}

bool WowExecuteRemoteProc64(int option, HANDLE hProcess, wchar_t* lpModuleName, char* lpProcName, wchar_t* argt, wchar_t** argv, int argc, DWORD& exitCode)
{ // FOR API: CreateRemoteThreads
    uint64_t procAddress = 0  ;
    if (wcscmp(lpModuleName, L"null") != 0) {
        HMODULE hModule = GetModuleHandleW(lpModuleName);
        if (hModule == nullptr)
            return false;
        procAddress = reinterpret_cast<uint64_t>(GetProcAddress(hModule, lpProcName));
        if (procAddress == 0)
            return false;
    }
    else {
        // lpProcName is the real remote function address.
        procAddress = strtoull(lpProcName, nullptr, 10);
    }

    int formatc = 0;
    if (argc == 0)
        return false;

    std::vector<RemoteArg> remoteArgs;
    bool result = true;
    split(argt, L';', [&](wchar_t* s, wchar_t* e) {
        if (formatc >= argc)
            return true; // break if arguments insufficient 

        auto n = e - s;

        /*
        #currently supported parameter types
        v; [void]
        s; [s]
        u64; [uint64_t]
        us; [UNICODE_STRING]
        */
        auto temp = s[n];
        s[n] = L'\0';
        RemoteArg remoteArg;
        if (wcscmp(s, L"u64") == 0)
        {
            remoteArg.type = 0;
            remoteArg.value.valueint = wcstoull(argv[formatc], nullptr, 10);
        }
        else if (wcscmp(s, L"s") == 0)
        {
            remoteArg.type = 1;
            auto string = transcode(argv[formatc]);
            remoteArg.value.valuestring = rpstrdup(hProcess, string);
            free(string);
        }
        else if (wcscmp(s, L"ws") == 0)
        {
            remoteArg.type = 2;
            remoteArg.value.valuewstring = rpwcsdup(hProcess, argv[formatc]);
        }
        else if (wcscmp(s, L"us") == 0)
        {
            remoteArg.type = 3;

            char* struc = (char*)rpalloc(hProcess, sizeof(UNICODE_STRING));

            WORD length = static_cast<WORD>(wcslen(argv[formatc]));
            auto totalBytes = (length + 1) * 2;
            auto rbuffer = rpalloc(hProcess, totalBytes);
            rpwrite(hProcess, rbuffer, argv[formatc], totalBytes);

            rpwrite(hProcess, struc + offsetof(UNICODE_STRING, MaximumLength), &totalBytes, 2);
            totalBytes -= sizeof(wchar_t);
            rpwrite(hProcess, struc + offsetof(UNICODE_STRING, Length), &totalBytes, 2);
            rpwrite(hProcess, struc + offsetof(UNICODE_STRING, Buffer), &rbuffer, sizeof(rbuffer));

            remoteArg.appendBuffer = rbuffer;
            remoteArg.value.ptr = struc;
        }
        else {
            // error
            result = false;
            return true;
        }

        s[n] = temp;
        ++formatc;

        remoteArgs.push_back(remoteArg);

        return false;
    });
    /*
    0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov         rcx,0; no need, cross transport

    The second operand MAX bytes is: 4Bytes
    Add & Sub rsp
    48 83 EC 20          sub         rsp,20h    1B
    48 81 EC 20 11 00 00 sub         rsp,1120h  4B

    48 83 C4 20          add         rsp,20h    1B
    48 81 C4 20 11 00 00 add         rsp,1120h  4B

    mov      qword ptr [rsp+?],rax, 1B or 4B
    48 89 84 24 + ?
    */

    const unsigned char rpc_thunk_code_template[] = {
#if _ENABLE_INJECT_DEBUG
        0xCC, // int 3
#endif
        0x48, 0x83, 0xEC, 0x28,                                         // sub         rsp,28h ; 4 parameters + ret address = 4 * 8 + 8 = 40 = 28h
        //0x49, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // mov         r9,0
        //0x49, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // mov         r8,0
        //0x48, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // mov         rdx,0
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // mov         rax,0; The remote Proc
        0xFF, 0xD0,                                                     // call rax
        0x48, 0x83, 0xC4, 0x28,                                         // add rsp, 28
        // 0x48, 0x89, 0xC1                                             // mov         rcx,rax // option = 1
        // 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov         rax,2222222222222222h; Address of RtlExitUserThread // option = 1
    };
    const unsigned char rpc_thunk_code_exit_0[] = {
        0xC3,                                                           // ret
        0x19, 0x88, 0x12, 0x19                                          // MAGIC 19881219
    };

    const unsigned char rpc_thunk_code_exit_1[] = {
        // 0x33, 0xC9,                                                  // xor         ecx,ecx; exit code: 0
        0x48, 0xFF, 0xE0,                                               // jmp         rax; jmp to RtlExitUserThread
        0x19, 0x88, 0x12, 0x19                                          // MAGIC 19881219
    };

    if (formatc > 1 || option != 0) { // The first parameter transfer by ThreadProc(void*/*rcx*/);
        if (formatc <= 4) { // argc = [2, 4]
            auto thunkSize = sizeof(rpc_thunk_code_template) + 10 * (formatc - 1) + 13/*mov rcx,rax & mov rax, u64*/ * option + (option == 0 ? sizeof(rpc_thunk_code_exit_0) : sizeof(rpc_thunk_code_exit_1));
            auto stackNeeded = calc_stack_size(formatc);

            auto thunkLocal = (unsigned char*)calloc(1, thunkSize);
            auto ptr = thunkLocal;
#if _ENABLE_INJECT_DEBUG
            *ptr++ = 0xCC;
#endif
            // sub rsp, stackNeeded;
            *ptr++ = 0x48;
            *ptr++ = 0x83;
            *ptr++ = 0xEC;
            *ptr++ = static_cast<uint8_t>(stackNeeded);

            if (formatc > 1) {
                // mov rdx, ?; 2nd arg
                *(uint16_t*)ptr = 0xBA48, ptr += sizeof(uint16_t);
                *(uint64_t*)ptr = remoteArgs[1].value.valueint, ptr += sizeof(uint64_t);
                if (formatc > 2) {
                    // mov r8, ?; 3rd arg
                    *(uint16_t*)ptr = 0xB849, ptr += sizeof(uint16_t);
                    *(uint64_t*)ptr = remoteArgs[2].value.valueint, ptr += sizeof(uint64_t);
                    if (formatc > 3) {
                        // mov r9, ? 4th arg
                        *(uint16_t*)ptr = 0xB949, ptr += sizeof(uint16_t);
                        *(uint64_t*)ptr = remoteArgs[3].value.valueint, ptr += sizeof(uint64_t);
                    }
                }
            }

            // mov rax, ?; Address of The remote Proc
            *(uint16_t*)ptr = 0xB848, ptr += sizeof(uint16_t);
            *(uint64_t*)ptr = procAddress, ptr += sizeof(uint64_t);

            // call rax; The remote Proc
            *ptr++ = 0xFF;
            *ptr++ = 0xD0;

            // add rsp, 28
            *ptr++ = 0x48;
            *ptr++ = 0x83;
            *ptr++ = 0xC4;
            *ptr++ = static_cast<uint8_t>(stackNeeded);

            if (0 == option) {
                memcpy(ptr, rpc_thunk_code_exit_0, sizeof(rpc_thunk_code_exit_0)), ptr += sizeof(rpc_thunk_code_exit_0);
            }
            else {
                // mov         rcx,rax // now rcx will as exitCode for call API: RtlExitUserThread(DWORD exitCode)
                *ptr++ = 0x48;
                *ptr++ = 0x89;
                *ptr++ = 0xC1;

                // mov  rax,?; Address of RtlExitUserThread
                *(uint16_t*)ptr = 0xB848, ptr += sizeof(uint16_t);
                *(uint64_t*)ptr = reinterpret_cast<uint64_t>(RtlExitUserThread), ptr += sizeof(uint64_t);
                memcpy(ptr, rpc_thunk_code_exit_1, sizeof(rpc_thunk_code_exit_1)), ptr += sizeof(rpc_thunk_code_exit_1);
            }

            assert(ptr - thunkLocal == thunkSize);
            auto finalThunkCode = rpmemdup(hProcess, thunkLocal, thunkSize, PAGE_EXECUTE_READWRITE);

            result = rpcall(option, hProcess, finalThunkCode, remoteArgs[0].value.ptr, exitCode);

            free(thunkLocal);
        }
        else { // > 4 parameters, TODO: implement
        }
    }
    else {
        // No thunk code needed, call directly
        result = rpcall(option, hProcess, reinterpret_cast<void*>(procAddress), remoteArgs[0].value.ptr, exitCode);
    }


    // free remote args
    for (auto& remoteArg : remoteArgs)
    {
        if (remoteArg.type != 0) {

            if (remoteArg.value.ptr != nullptr)
                rpfree(hProcess, remoteArg.value.ptr);

            if (remoteArg.appendBuffer != nullptr)
                rpfree(hProcess, remoteArg.appendBuffer);
        }
    }

    return result;
}
