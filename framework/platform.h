#pragma once
#include <string>

#ifdef VK_USE_PLATFORM_WIN32_KHR
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOGID
        #define NOGDI
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#include <windows.h>
#endif

#ifdef QT_CORE_LIB
    #include <QtGlobal>
#endif

#include <xmmintrin.h>

template<size_t alignment>
class alignas(alignment) AlignAs
{
public:
    void *operator new(size_t size)
    {
        void *ptr = _mm_malloc(size, alignment);
        if (!ptr)
            throw std::bad_alloc();
        return ptr;
    }

    void operator delete(void *ptr) noexcept
    {
        _mm_free(ptr);
    }
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
