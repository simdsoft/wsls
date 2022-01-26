// Copyright (c) 2017~2022 Simdsoft Limited - All Rights Reserved.
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

#if defined(_WIN64)
#define wcs_to_uptr wcstoull
#define str_to_uptr strtoull
#else
#define wcs_to_uptr wcstoul
#define str_to_uptr strtoul
#endif

enum OptionFlag
{
	OptionFlagNull, // Use CreateRemoteThread
	OptionFlagRaw = 1, // Use RtlCreateUserThread/RtlExitUserThread
	OptionFlagReturnBool = 2, // Convert Result as Bool value
};

typedef int(NTAPI* RtlCreateUserThreadProc)(
	void*,					// ProcessHandle
	void*,                  // SecurityDescriptor
	uintptr_t,                // CreateSuspended
	uintptr_t,                // StackZeroBits
	void*,                  // StackReserved
	void*,                  // StackCommit
	void*,		            // StartAddress
	void* StartParameter,   // StartParameter
	PHANDLE,                // ThreadHandle
	void* lpClientID    // ClientID)
	);
typedef int(NTAPI* RtlExitUserThreadProc)(NTSTATUS);

typedef DWORD(NTAPI* ZwSuspendProcessProc)(HANDLE);
typedef DWORD(NTAPI* ZwResumeProcessProc)(HANDLE);


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

typedef _UNICODE_STRING_T<wchar_t*> UNICODE_STRING;
typedef UNICODE_STRING* PUNICODE_STRING;

enum {
	RAT_INT,
	RAT_STRING,
	RAT_WSTRING,
	RAT_USTRING,
};

struct RemoteArg
{
	int type = 0; // 0: integer, 1: string, 2: wstring, 3: UNICODE_STRING
	union {
		char* s;
		wchar_t* ws;
		uintptr_t uptr = 0;
		void* vptr;
		UNICODE_STRING* us;
	} value;

	// The remote buffer store the unicode string content
	void* rbuf = nullptr;
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
BOOL rpwrite(HANDLE hProcess, void* p, const void* data, size_t size)
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

bool rpcall(int option, HANDLE hProcess, void* StartAddress, void* StartParameter, DWORD& exitCode);

/*

usage:
	  wow64helper.exe 0 PID OSModuleName ModuleProcName paramsTypes [parameters]...
	  wow64helper.exe 1 PID OSModuleName ModuleProcName paramsTypes [parameters]...

 #currently supported parameter types
	 v; [void]
	 uptr/u64; [same with uint64_t on win64, same with uint32_t on win32]
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
RtlExitUserThreadProc   RtlExitUserThread;
ZwSuspendProcessProc    ZwSuspendProcess;
ZwResumeProcessProc     ZwResumeProcess;

//typedef
//NTSTATUS
//(NTAPI
//	* LdrLoadDllProc)(
//
//		IN PWCHAR               PathToFile OPTIONAL,
//		IN PULONG               Flags OPTIONAL,
//		IN PUNICODE_STRING      ModuleFileName,
//		OUT PHANDLE             ModuleHandle);

// LdrLoadDllProc LdrLoadDll;
int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
#if _WAITING_DEBUGGER_ATTACH
	MessageBox(nullptr, L"waiting for debugger to attach!", L"Tips", MB_OK);
#endif

	int ret = -1;

	auto ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll != nullptr) {
		RtlCreateUserThread = (RtlCreateUserThreadProc)GetProcAddress(ntdll, "RtlCreateUserThread");
		RtlExitUserThread = (RtlExitUserThreadProc)GetProcAddress(ntdll, "RtlExitUserThread");
		ZwSuspendProcess = (ZwSuspendProcessProc)GetProcAddress(ntdll, "ZwSuspendProcess");
		ZwResumeProcess = (ZwResumeProcessProc)GetProcAddress(ntdll, "ZwResumeProcess");
		// LdrLoadDll = (LdrLoadDllProc)GetProcAddress(ntdll, "LdrLoadDll");
	}

	auto szCmdLine = GetCommandLine();

	int argc = 0;
	auto argv = CommandLineToArgvW(szCmdLine, &argc);

	if (argc >= 6) {
		int option = wcstol(argv[1], nullptr, 10);

		DWORD dwPID = wcstoul(argv[2], nullptr, 10);
		HANDLE hProcess;

		bool bOtherProcess = dwPID != 0xffffffff;
		if (bOtherProcess)
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
		else
			hProcess = GetCurrentProcess(); // support self

		if (hProcess == nullptr)
			return ret;

		if (bOtherProcess)
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
			uintptr_t UniqueProcess;
			uintptr_t UniqueThread;
		} clientId;

		HANDLE hRemoteThread = INVALID_HANDLE_VALUE;
		int ret = RtlCreateUserThread(
			hProcess,				 // ProcessHandle
			nullptr,                 // SecurityDescriptor
			(uintptr_t)FALSE,          // CreateSuspended
			(uintptr_t)0,              // StackZeroBits
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
	uintptr_t procAddress = 0;
	if (wcscmp(lpModuleName, L"null") != 0) {
		HMODULE hModule = GetModuleHandleW(lpModuleName);
		if (hModule == nullptr)
			return false;
		procAddress = reinterpret_cast<uintptr_t>(GetProcAddress(hModule, lpProcName));
		if (procAddress == 0)
			return false;
	}
	else {
		// lpProcName is the real remote function address.
		procAddress = str_to_uptr(lpProcName, nullptr, 10);
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
		uptr; [Win64 same with uint64_t]
		us; [UNICODE_STRING]
		*/
		auto temp = s[n];
		s[n] = L'\0';
		RemoteArg remoteArg;
		if (wcscmp(s, L"u64") == 0 || wcscmp(s, L"uptr") == 0)
		{
			remoteArg.type = 0;
			remoteArg.value.uptr = wcs_to_uptr(argv[formatc], nullptr, 10);
		}
		else if (wcscmp(s, L"s") == 0)
		{
			remoteArg.type = 1;
			auto string = transcode(argv[formatc]);
			remoteArg.value.s = rpstrdup(hProcess, string);
			free(string);
		}
		else if (wcscmp(s, L"ws") == 0)
		{
			remoteArg.type = 2;
			remoteArg.value.ws = rpwcsdup(hProcess, argv[formatc]);
		}
		else if (wcscmp(s, L"us") == 0)
		{
			remoteArg.type = 3;

			uint8_t* us = (uint8_t*)rpalloc(hProcess, sizeof(UNICODE_STRING));

			WORD length = static_cast<WORD>(wcslen(argv[formatc]));
			auto totalBytes = (length + 1) * 2;
			auto rbuffer = rpalloc(hProcess, totalBytes);
			rpwrite(hProcess, rbuffer, argv[formatc], totalBytes);

			rpwrite(hProcess, us + offsetof(UNICODE_STRING, MaximumLength), &totalBytes, 2);
			totalBytes -= sizeof(wchar_t);
			rpwrite(hProcess, us + offsetof(UNICODE_STRING, Length), &totalBytes, 2);
			rpwrite(hProcess, us + offsetof(UNICODE_STRING, Buffer), &rbuffer, sizeof(rbuffer));

			remoteArg.value.us = (UNICODE_STRING*)us;
			remoteArg.rbuf = rbuffer;
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
	0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov         rcx,0; no need, Passthrough parameters

	The second operand MAX bytes is: 4Bytes
	Add & Sub rsp
	48 83 EC 20          sub         rsp,20h    1B
	48 81 EC 20 11 00 00 sub         rsp,1120h  4B

	48 83 C4 20          add         rsp,20h    1B
	48 81 C4 20 11 00 00 add         rsp,1120h  4B

	mov      qword ptr [rsp+?],rax, 1B or 4B
	48 89 84 24 + ?
	*/

#if defined(_WIN64)
	// Windows X64 Calling Convention the first 4 parameter use rcx,rdx,r8,r9
	// > 4, use stack right --> left
	const unsigned char rpc_thunk_code_template[] = {
#if _ENABLE_INJECT_DEBUG
		0xCC, // int 3
#endif
		0x48, 0x83, 0xEC, 0x28,                                         // sub         rsp,28h ; 4 parameters + ret address = 4 * 8 + 8 = 40 = 28h
		//0x49, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // mov         r9,0
		//0x49, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // mov         r8,0
		//0x48, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // mov         rdx,0
		// ----- 0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // mov         rcx,0; no need, Passthrough parameters
		0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // mov         rax,0; The remote Proc
		0xFF, 0xD0,                                                     // call rax
		0x48, 0x83, 0xC4, 0x28,                                         // add rsp, 28
		// 0x48, 0x89, 0xC1                                             // mov         rcx,rax // option = 1, pass to RtlExituserThread(Status)
		// 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov         rax,2222222222222222h; Address of RtlExitUserThread // option = 1
	};
	const unsigned char rpc_thunk_code_exit_0[] = {
		0xC3,                                                           // ret
		0x19, 0x88, 0x12, 0x19                                          // MAGIC 19881219
	};

	const unsigned char rpc_thunk_code_exit_1[] = {
		0x48, 0xFF, 0xE0,                                               // jmp         rax; jmp to RtlExitUserThread
		0x19, 0x88, 0x12, 0x19                                          // MAGIC 19881219
	};

	if (formatc > 1 || option != 0) { // The first parameter transfer by ThreadProc(void*/*rcx*/);
		if (formatc <= 4) { // argc = [2, 4]
			auto thunkSize = sizeof(rpc_thunk_code_template) + 10 * (formatc - 1) + (option == 0 ? sizeof(rpc_thunk_code_exit_0) : sizeof(rpc_thunk_code_exit_1));
			if (option != 0) thunkSize += 13; /*for code: mov rcx,rax & mov rax, u64*/
			auto stackNeeded = calc_stack_size(formatc);

			auto thunkLocal = (unsigned char*)calloc(1, thunkSize);
			auto ptr = thunkLocal;
#if _ENABLE_INJECT_DEBUG
			* ptr++ = 0xCC;
#endif
			// sub rsp, stackNeeded;
			* ptr++ = 0x48;
			*ptr++ = 0x83;
			*ptr++ = 0xEC;
			*ptr++ = static_cast<uint8_t>(stackNeeded);

			// !!!The first parameter pass by thread function argument, and use rcx, so no need to assign value again.

			if (formatc > 1) {
				// mov rdx, ?; 2nd arg
				*(uint16_t*)ptr = 0xBA48, ptr += sizeof(uint16_t);
				*(uintptr_t*)ptr = remoteArgs[1].value.uptr, ptr += sizeof(uintptr_t);
				if (formatc > 2) {
					// mov r8, ?; 3rd arg
					*(uint16_t*)ptr = 0xB849, ptr += sizeof(uint16_t);
					*(uintptr_t*)ptr = remoteArgs[2].value.uptr, ptr += sizeof(uintptr_t);
					if (formatc > 3) {
						// mov r9, ? 4th arg
						*(uint16_t*)ptr = 0xB949, ptr += sizeof(uint16_t);
						*(uintptr_t*)ptr = remoteArgs[3].value.uptr, ptr += sizeof(uintptr_t);
					}
				}
			}

			// mov rax, ?; Address of The remote Proc
			*(uint16_t*)ptr = 0xB848, ptr += sizeof(uint16_t);
			*(uintptr_t*)ptr = procAddress, ptr += sizeof(uintptr_t);

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
				*(uintptr_t*)ptr = reinterpret_cast<uintptr_t>(RtlExitUserThread), ptr += sizeof(uintptr_t);
				memcpy(ptr, rpc_thunk_code_exit_1, sizeof(rpc_thunk_code_exit_1)), ptr += sizeof(rpc_thunk_code_exit_1);
			}

			assert(ptr - thunkLocal == thunkSize);
			auto finalThunkCode = rpmemdup(hProcess, thunkLocal, thunkSize, PAGE_EXECUTE_READWRITE);

			result = rpcall(option, hProcess, finalThunkCode, remoteArgs[0].value.vptr, exitCode);

			free(thunkLocal);
		}
		else { // > 4 parameters, TODO: implement
		}
	}
	else {
		// No thunk code needed, call directly
		result = rpcall(option, hProcess, reinterpret_cast<void*>(procAddress), remoteArgs[0].value.vptr, exitCode);
	}
#else // Win32
	// All Win32 API is __stdcall
	// means: Use stack for passing parameters, right --> left
	/*
	* 00EF1072 68 FF FF FF 7F     push        7FFFFFFFh
	00EF1077 68 FF FF FF 7F       push        7FFFFFFFh
	00EF107C 68 FF FF FF 7F       push        7FFFFFFFh
	00EF1081 68 FF FF FF 7F       push        7FFFFFFFh
	001B1086 FF D0                call        eax; The remote Proc address
	00A3100D C2 0C 00             ret         0Ch
	*/
	//00C5A8E8 B9 FF FF FF 7F       mov         ecx, 7FFFFFFFh
	//00C5A8ED 50                   push        eax
	//00C5A8EE FF E1                jmp         ecx
	//0079A8E8 FF D1                call        ecx
	//__asm mov ecx, 0x7fffffff;
	//__asm push eax;
	//__asm jmp ecx;
	//_asm call ecx;

	const unsigned char rpc_thunk_code_template[] = {
	#if _ENABLE_INJECT_DEBUG
			0xCC, // int 3
	#endif
			// --- 0x68, 0x00, 0x00, 0x00, 0x00,   // push param4; param4
			//0x68, 0x00, 0x00, 0x00, 0x00,   // push param3; param3
			//0x68, 0x00, 0x00, 0x00, 0x00,   // push param2; param2
			//0x68, 0x00, 0x00, 0x00, 0x00,   // push param1; param3
			0xB8, 0x00, 0x00, 0x00, 0x00,     // mov eax, remoteProcAddress The remote Proc
			0xFF, 0xD0,                       // call eax, after return, eax is the ret-val of remtoe proc
			// 0xB9, 0x00, 0x00, 0x00, 0x00,  // mov  ecx,2222222222222222h; Address of RtlExitUserThread // option = 1
	};

	// eax store the return value
	const unsigned char rpc_thunk_code_exit_0[] = {
		0xC2, 0x04, 0x00,     // ret, 4; no need, because we don't push the last parameter again passed by threadFunc parameter
		0x19, 0x88, 0x12, 0x19  // MAGIC 19881219
	};

	// eax store the return value
	const unsigned char rpc_thunk_code_exit_1[] = {
		0x50,                       // push eax
		0xFF, 0xD1,                 // jmp ecx; the RtlExitUserThread
		0x19, 0x88, 0x12, 0x19      // MAGIC 19881219
	};

	if (formatc > 1 || option != 0) { // The first parameter transfer by ThreadProc(void*/*rcx*/);
		auto thunkSize = sizeof(rpc_thunk_code_template) + 5 * (formatc)+(option == 0 ? sizeof(rpc_thunk_code_exit_0) : sizeof(rpc_thunk_code_exit_1));
		if (option != 0) {
			thunkSize += 5; // for exit code pass to RtlExitUserThread
		}
		// auto stackNeeded = calc_stack_size(formatc);

		auto thunkLocal = (unsigned char*)calloc(1, thunkSize);
		auto ptr = thunkLocal;
#if _ENABLE_INJECT_DEBUG
		* ptr++ = 0xCC;
#endif
		// sub rsp, stackNeeded
		// * ptr++ = 0x48;
		// *ptr++ = 0x83;
		// *ptr++ = 0xEC;
		// *ptr++ = static_cast<uint8_t>(stackNeeded);

		if (formatc > 0) { // push parameter n~2
			for (int idx = formatc - 1; idx >= 0; --idx) {
				*ptr++ = 0x68;
				*(uintptr_t*)ptr = remoteArgs[idx].value.uptr, ptr += sizeof(uintptr_t);
			}
		}

		// mov eax, ?; Address of The remote Proc
		*ptr++ = 0xB8;
		*(uintptr_t*)ptr = procAddress, ptr += sizeof(uintptr_t);

		// call eax; The remote Proc
		*ptr++ = 0xFF;
		*ptr++ = 0xD0;

		// add rsp, 28
		// *ptr++ = 0x48;
		// *ptr++ = 0x83;
		// *ptr++ = 0xC4;
		// *ptr++ = static_cast<uint8_t>(stackNeeded);

		if (0 == option) {
			memcpy(ptr, rpc_thunk_code_exit_0, sizeof(rpc_thunk_code_exit_0)), ptr += sizeof(rpc_thunk_code_exit_0);
		}
		else {
			// mov  ecx,eax; now ecx will as exitCode for call API: RtlExitUserThread(DWORD exitCode)
			// *ptr++ = 0x89;
			// *ptr++ = 0xC1;

			// mov  eax,?; Address of RtlExitUserThread
			*ptr++ = 0xB9;
			*(uintptr_t*)ptr = reinterpret_cast<uintptr_t>(RtlExitUserThread), ptr += sizeof(uintptr_t);

			// the return code
			memcpy(ptr, rpc_thunk_code_exit_1, sizeof(rpc_thunk_code_exit_1)), ptr += sizeof(rpc_thunk_code_exit_1);
		}

		assert(ptr - thunkLocal == thunkSize);
		auto finalThunkCode = rpmemdup(hProcess, thunkLocal, thunkSize, PAGE_EXECUTE_READWRITE);

		result = rpcall(option, hProcess, finalThunkCode, remoteArgs[0].value.vptr, exitCode);

		free(thunkLocal);
	}
	else {
		// No thunk code needed, call directly
		result = rpcall(option, hProcess, reinterpret_cast<void*>(procAddress), remoteArgs[0].value.vptr, exitCode);
	}
#endif

	// free remote args
	for (auto& remoteArg : remoteArgs)
	{
		if (remoteArg.type != 0) {
			if (remoteArg.value.vptr != nullptr)
				rpfree(hProcess, remoteArg.value.vptr);

			if (remoteArg.rbuf != nullptr)
				rpfree(hProcess, remoteArg.rbuf);
		}
	}

	return result;
}
