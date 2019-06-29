#pragma once
#include <memory>
#include "nonCopyable.h"

namespace magma
{
    class CommandBuffer;
    class VertexBuffer;
    class IndexBuffer;
    class SrcTransferBuffer;
    struct VertexInputState;
}

class Mesh : public NonCopyable
{
public:
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const = 0;
    virtual const magma::VertexInputState& getVertexInput() const = 0;
};
