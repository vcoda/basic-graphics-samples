#pragma once
#include <xmmintrin.h>

#ifndef _DECLSPEC_ALLOCATOR
#define _DECLSPEC_ALLOCATOR
#endif

namespace utilities
{
        // CLASS TEMPLATE allocator
template<class _Ty>
    class aligned_allocator
    {    // generic allocator for objects of class _Ty
public:
#ifdef _MSC_VER
    static_assert(!std::is_const_v<_Ty>,
            "The C++ Standard forbids containers of const elements "
            "because allocator<const T> is ill-formed.");
#endif
    using _Not_user_specialized = void;

    using value_type = _Ty;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template<class _Other>
    using other = aligned_allocator<_Other>;

    aligned_allocator() noexcept
        {    // construct default allocator (do nothing)
        }

    aligned_allocator(const aligned_allocator&) noexcept = default;
    template<class _Other>
        aligned_allocator(const aligned_allocator<_Other>&) noexcept
        {    // construct from a related allocator (do nothing)
        }

    void deallocate(_Ty * const _Ptr, const size_t /* _Count */)
        {    // deallocate object at _Ptr
        _mm_free(_Ptr);
        }

    _DECLSPEC_ALLOCATOR _Ty * allocate(const size_t _Count)
        {    // allocate array of _Count elements
        void* ptr = _mm_malloc(_Count * sizeof(_Ty), 16);
                return reinterpret_cast<_Ty *>(ptr);
        }
    };

    // http://en.cppreference.com/w/cpp/memory/allocator/operator_cmp
    template<class _Ty,
        class _Other> inline
        bool operator==(const aligned_allocator<_Ty>&,
                const aligned_allocator<_Other>&) noexcept
        {    // test for allocator equality
            return (true);
        }

    template<class _Ty,
        class _Other> inline
        bool operator!=(const aligned_allocator<_Ty>&,
                const aligned_allocator<_Other>&) noexcept
        {    // test for allocator inequality
            return (false);
        }
} // namespace utilities
