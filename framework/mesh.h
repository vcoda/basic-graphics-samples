#pragma once
#include <memory>
#include "magma/internal/noncopyable.h"

namespace magma
{
    class CommandBuffer;
    class VertexBuffer;
    class IndexBuffer;
    class SrcTransferBuffer;
    struct VertexInputState;
}

class Mesh : public magma::internal::NonCopyable
{
public:
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const = 0;
    virtual const magma::VertexInputState& getVertexInput() const = 0;
};
