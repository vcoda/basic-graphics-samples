#pragma once
#include "mesh.h"

class CubeMesh : public Mesh
{
public:
    CubeMesh(std::shared_ptr<magma::CommandBuffer> cmdBuffer);
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const override;
    virtual const magma::VertexInputState& getVertexInput() const override;

private:
    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::VertexBuffer> texCoordBuffer;
};
