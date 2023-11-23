#include "magma/core/pch.h"
#include "linearAllocator.h"

#ifdef _MSC_VER
    #define MALLOC(size) _mm_malloc(size, MAGMA_ALIGNMENT)
    #define FREE(p) _mm_free(p)
#else
    #define MAGMA_MALLOC(size) malloc(size)
    #define MAGMA_FREE(p) free(p)
#endif // _MSC_VER

LinearAllocator::LinearAllocator():
    bufferSize(1024 * 64),
    buffer(MALLOC(bufferSize)),
    head((char *)buffer),
    bytesAllocated(0)
{}

LinearAllocator::~LinearAllocator()
{
    FREE(buffer);
}

void *LinearAllocator::alloc(size_t size)
{
    void *p = nullptr;
    if (size)
    {
        if (size + bytesAllocated <= bufferSize)
        {   // Simply eat new chunk from fixed buffer until overflow
            p = head;
            head = (char *)MAGMA_ALIGN((intptr_t)(head + size));
        }
        else
        {   // Use malloc() on overflow
            p = MALLOC(size);
        }
        MAGMA_ASSERT(MAGMA_ALIGNED(p));
        bytesAllocated += size;
        blocks[p] = size;
    }
    return p;
}

void LinearAllocator::free(void *p) noexcept
{
    if (p)
    {
        auto it = blocks.find(p);
        if (it != blocks.end())
        {
            bytesAllocated -= it->second;
            blocks.erase(it);
        }
        if (p < buffer || p > head)
            FREE(p);
        else
            // Do nothing
            ;
    }
}

size_t LinearAllocator::getBytesAllocated() const noexcept
{
    return bytesAllocated;
}
