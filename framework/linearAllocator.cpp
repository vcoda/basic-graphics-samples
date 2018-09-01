#include "linearAllocator.h"
#include "magma/shared.h"

LinearAllocator::LinearAllocator():
    bufferSize(1024 * 64),
    buffer(MAGMA_MALLOC(bufferSize)),
    head((char *)buffer),
    bytesAllocated(0)
{}

LinearAllocator::~LinearAllocator()
{
    MAGMA_FREE(buffer);
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
            p = MAGMA_MALLOC(size);
        }
        MAGMA_ASSERT(MAGMA_ALIGNED(p));
        bytesAllocated += size;
        blocks[p] = size;
    }
    return p;
}

void LinearAllocator::free(void *p)
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
            ::MAGMA_FREE(p);
        else
            // Do nothing
            ;
    }
}

size_t LinearAllocator::getBytesAllocated() const
{
    return bytesAllocated;
}
