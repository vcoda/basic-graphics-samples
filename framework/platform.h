#pragma once
#include <string>

#ifndef TEXT
#ifdef UNICODE
#define TEXT(s) L##s
#else
#define TEXT(s) s
#endif
#endif // !TEXT

#ifdef UNICODE
typedef const wchar_t *String;
#else
typedef const char *String;
#endif

namespace std
{
#ifdef UNICODE
    typedef std::basic_string<wchar_t> tstring;
#else
    typedef std::basic_string<char> tstring;
#endif
    template <typename T>
    inline tstring to_tstring(T val)
    {
#ifdef UNICODE
        return std::to_wstring(val);
#else
        return std::to_string(val);
#endif
    }
}

