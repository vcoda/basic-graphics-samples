#pragma once
#include "magma/allocator/objectAllocator.h"

class LinearAllocator : public magma::IObjectAllocator
{
public:
    LinearAllocator();
    ~LinearAllocator();
    virtual void *alloc(size_t size) override;
    virtual void free(void *p) override;
    virtual size_t getBytesAllocated() const override;

private:
    const size_t bufferSize;
    void *const buffer;
    char *head;
    size_t bytesAllocated;
};