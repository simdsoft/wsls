// wsls-arch.cpp : This file contains the 'main' function. Program execution begins and ends there.
// 

#include <iostream>
#include <Windows.h>
#include "../libwsls/ntcvt.hpp"

enum {
    IMAGE_ARCH_UNKNOWN,
    IMAGE_ARCH_X86 = 86,
    IMAGE_ARCH_X64 = 64,
};

static int detectExeArch(const std::wstring& filePath) {
    LPVOID LpAddr;
    IMAGE_DOS_HEADER* pDosHeader;
    IMAGE_NT_HEADERS* pNT_Header;
    IMAGE_FILE_HEADER* pImage_File_Head;
    IMAGE_OPTIONAL_HEADER32* pImage_Optional_Header;

    HANDLE m_Handle = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (m_Handle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_Handle);
        return false;
    }
    HANDLE m_hMmap = CreateFileMappingA(m_Handle, 0, PAGE_READONLY, 0, 0, NULL);
    if (m_hMmap == NULL)
    {
        CloseHandle(m_Handle);
        CloseHandle(m_hMmap);
        return false;
    }
    LpAddr = MapViewOfFile(m_hMmap, FILE_MAP_READ, 0, 0, 0);
    if (LpAddr == NULL) //Ó³Ïñ»ùÖ·
    {
        CloseHandle(m_Handle);
        CloseHandle(m_hMmap);
        return false;
    }
    CloseHandle(m_Handle);
    CloseHandle(m_hMmap);

    pDosHeader = (IMAGE_DOS_HEADER*)LpAddr;

    pNT_Header = (IMAGE_NT_HEADERS*)((BYTE*)LpAddr + pDosHeader->e_lfanew);

    // vlog::logd("Signature: %s\n", (char *)&pNT_Header->Signature);
    if (pNT_Header->Signature != IMAGE_NT_SIGNATURE && pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return false;
    }

    pImage_File_Head = (IMAGE_FILE_HEADER*)((BYTE*)LpAddr + pDosHeader->e_lfanew + 4);
    pImage_Optional_Header =
        (IMAGE_OPTIONAL_HEADER32*)((BYTE*)LpAddr + pDosHeader->e_lfanew + 4 + 20);

    bool bX86 = false;
    if (pImage_File_Head->Machine == IMAGE_FILE_MACHINE_I386)
    {
        // vlog::logd("Machine: Intel 386\n");
        return IMAGE_ARCH_X86;
    }
    else if (pImage_File_Head->Machine == IMAGE_FILE_MACHINE_AMD64) {
        return IMAGE_ARCH_X64;
    }
    else return IMAGE_ARCH_UNKNOWN;
}

int main(int argc, char** argv)
{
    if (argc >= 2) {
        return detectExeArch(ntcvt::from_chars(argv[1]));
    }
    return -1;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
