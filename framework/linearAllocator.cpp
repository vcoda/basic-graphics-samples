#include <memory>
#include "linearAllocator.h"

LinearAllocator::LinearAllocator():
    bufferSize(1024 * 64),
    buffer(calloc(bufferSize, 1)),
    head((char *)buffer),
    bytesAllocated(0)
{}

LinearAllocator::~LinearAllocator()
{
    free(buffer);
}

void *LinearAllocator::alloc(size_t size)
{
    void *p;
    if (size + bytesAllocated <= bufferSize)
    {   // Simply eat new chunk from fixed buffer until overflow
        p = head;
        head += size;
    }
    else
    {   // Use malloc() on overflow
        p = malloc(size);
    }
    bytesAllocated += size;
    return p;
}

void LinearAllocator::free(void *p)
{
    if (p < buffer || p > head)
        ::free(p);
    else
        // Do nothing
        ;
}

size_t LinearAllocator::getBytesAllocated() const
{
    return bytesAllocated;
}
