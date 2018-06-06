#pragma once
#include <Windows.h>
#include <string>
#include <string_view>

namespace wsls {
    /*--- This a C++ universal sprintf in the future.
    **  @pitfall: The behavior of vsnprintf between VS2013 and VS2015/2017 is different
    **      VS2013 or Unix-Like System will return -1 when buffer not enough, but VS2015/2017 will return the actural needed length for buffer at this station
    **      The _vsnprintf behavior is compatible API which always return -1 when buffer isn't enough at VS2013/2015/2017
    **      Yes, The vsnprintf is more efficient implemented by MSVC 19.0 or later, AND it's also standard-compliant, see reference: http://www.cplusplus.com/reference/cstdio/vsnprintf/
    */
    std::string sfmt(const char* format, ...);
    std::wstring sfmt(const wchar_t* format, ...);
    bool replace_once(std::string& string, const std::string& replaced_key, const std::string& replacing_key);
    bool replace_once(std::wstring& string, const std::wstring& replaced_key, const std::wstring& replacing_key);
    int replace(std::wstring& string, const std::wstring& replaced_key, const std::wstring& replacing_key);
    int make_bridge(const wchar_t* shell, const wchar_t* app);
    std::string readFileData(const char* fileName);
	void writeFileData(const char* fileName, const std::string& content);
    void convertPathToWinStyle(std::string& path, size_t offset = 0);
	void convertPathToWinStyle(std::wstring& path, size_t offset = 0);
	void convertPathToUnixStyle(std::wstring& path, size_t offset = 0);
	bool isExecFileExist(const wchar_t* _FileName);
    std::wstring transcode$IL(std::string_view mcb, UINT cp = CP_ACP);
    std::string transcode$IL(std::wstring_view mcb, UINT cp = CP_ACP);
    
    std::wstring makeStyledPath(const char* _FileName);
    std::wstring makeStyledPath(const wchar_t* _FileName);

    bool isFileExists(const wchar_t* _Path);
    bool isDirectoryExists(const wchar_t* _Path);
    bool isAbsolutePath(const wchar_t* strPath);

    int mkdir(std::wstring&& _Path);
	
	template<typename _Elem> inline
    bool isStyledLongPath(const _Elem* _Path)
    {
        return _Path[0] == '\\' && _Path[1] == '\\' && _Path[2] == '?' && _Path[3] == '\\';
    }
};
