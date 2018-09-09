#pragma once
#include <map>
#include "magma/allocator/objectAllocator.h"

class LinearAllocator : public magma::IObjectAllocator
{
public:
    LinearAllocator();
    ~LinearAllocator();
    virtual void *alloc(size_t size) override;
    virtual void free(void *p) noexcept override;
    virtual size_t getBytesAllocated() const noexcept override;

private:
    const size_t bufferSize;
    void *const buffer;
    char *head;
    size_t bytesAllocated;
    std::map<void *, size_t> blocks;
};
