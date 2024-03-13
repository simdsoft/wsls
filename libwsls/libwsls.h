#pragma once
#include <Windows.h>
#include <string>
#include <string_view>
#include "ntcvt.hpp"

#define WSLS_MAX_PATH 4096

namespace wsls {
using namespace ntcvt;
/*--- This a C++ universal sprintf in the future.
    **  @pitfall: The behavior of vsnprintf between VS2013 and VS2015/2017 is different
    **      VS2013 or Unix-Like System will return -1 when buffer not enough, but VS2015/2017 will return the actural needed length for buffer at this station
    **      The _vsnprintf behavior is compatible API which always return -1 when buffer isn't enough at VS2013/2015/2017
    **      Yes, The vsnprintf is more efficient implemented by MSVC 19.0 or later, AND it's also standard-compliant, see reference: http://www.cplusplus.com/reference/cstdio/vsnprintf/
    */
std::string sfmt(const char* format, ...);
std::wstring sfmt(const wchar_t* format, ...);
bool replace_once(std::string& string, const std::string& replaced_key, const std::string& replacing_key, size_t offset = 0);
bool replace_once(std::wstring& string, const std::wstring& replaced_key, const std::wstring& replacing_key, size_t offset = 0);
int replace(std::wstring& string, const std::wstring& replaced_key, const std::wstring& replacing_key);
int make_bridge(const wchar_t* shell, const wchar_t* app, bool forward_mode);
std::string readFileData(const char* fileName);
int writeFileData(const char* fileName, const std::string& content, bool append = false);
void convertPathToWinStyle(std::string& path, size_t offset = 0);
void convertPathToWinStyle(std::wstring& path, size_t offset = 0);
void convertPathToUnixStyle(std::wstring& path, size_t offset = 0);
bool isExecFileExist(const wchar_t* _FileName);

std::wstring makeStyledPath(const char* _FileName);
std::wstring makeStyledPath(const wchar_t* _FileName);

bool isFileExists(const wchar_t* _Path);
bool isDirectoryExists(const wchar_t* _Path);
template<typename _Elem> inline
bool isAbsolutePath(const _Elem* strPath)
{
    return (((strPath[0] >= 'a' && strPath[0] <= 'z') || (strPath[0] >= 'A' && strPath[0] <= 'Z'))
        && strPath[1] == ':');
}

const char* getFileShortName(std::string_view _FileName);

int mkdir(std::wstring&& _Path);

template <typename _Elem>
inline bool hasUNCPrefix(const _Elem* _Path)
{
    return _Path[0] == '\\' && _Path[1] == '\\' && _Path[2] == '?' && _Path[3] == '\\';
}

template <typename _Elem>
inline bool isStyledWindowsPath(const _Elem* _Path)
{
    int slashs = 0;
    int dots = 0;
    enum {
        normal = 1,
        back_slash,
        back_slash_dot,
    } state = normal;
    while (*_Path) {
        switch (*_Path) {
        case '/':
            return false; // Have forward slash, not a Styled Windows Path
        case '\\':
            if (state == normal) {
                state = back_slash;
            } else { // state == back_slash || state == back_slash_dot, not a Styled Windows Path
                return false;
            }
            break;
        case '.':
            if (state == back_slash) {
                state = back_slash_dot;
            }
            break;
        default:
            state = normal;
            break;
        }
        ++_Path;
    }
    return true;
}
};
