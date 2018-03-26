#pragma once
#include <memory>

namespace magma
{
    class CommandBuffer;
    class VertexBuffer;
    class IndexBuffer;
}

class Mesh
{
public:
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const = 0;
};
