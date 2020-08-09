// wsls-hash.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "../libwsls/ntcvt.hpp"
#include "xxhash.h"

// 1MB
#define READ_FILE_BUF_SIZE (1*1024*1024)

int main(int argc, char** argv)
{
    if (argc >= 2) {
        HANDLE fileHandle = ::CreateFileW(ntcvt::from_chars(argv[1]).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, nullptr);
        if (fileHandle != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER fileLength = { 0 };
            if (!GetFileSizeEx(fileHandle, &fileLength) || fileLength.QuadPart == 0) return -2;

            XXH64_state_t* s = XXH64_createState();
            XXH64_reset(s, 0xdeadbeef);

            DWORD bytesRead = 0;
            char* buffer = (char*)malloc(READ_FILE_BUF_SIZE);
            while (ReadFile(fileHandle, buffer, READ_FILE_BUF_SIZE, &bytesRead, nullptr) && bytesRead > 0)
            {
                XXH64_update(s, buffer, bytesRead);
            }
            free(buffer);
            auto hashValue = (uint64_t)XXH64_digest(s);
            XXH64_freeState(s);

            CloseHandle(fileHandle);

            printf("%llu", hashValue);
            return 0;
        }
    }
    printf("%d", -1);
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
