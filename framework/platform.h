#pragma once
#include <xmmintrin.h>
#include <string>

template<size_t alignment>
struct alignas(alignment) Aligned
{
    void *operator new(size_t size) noexcept
        { return _mm_malloc(size, alignment); }
    void operator delete(void *p) noexcept
        { _mm_free(p); }
};

#ifndef TEXT
#ifdef UNICODE
#define TEXT(s) L##s
#else
#define TEXT(s) s
#endif
#endif // !TEXT

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
