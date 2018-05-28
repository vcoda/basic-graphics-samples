#pragma once
#include "mesh.h"

class PlaneMesh : public Mesh
{
public:
    PlaneMesh(float width, float height, bool textured, bool twoSided, std::shared_ptr<magma::CommandBuffer> cmdBuffer);
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const override;
    virtual const magma::VertexInputState& getVertexInput() const override;

private:
    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::VertexBuffer> normalBuffer;
    std::shared_ptr<magma::VertexBuffer> texCoordBuffer;
    std::shared_ptr<magma::IndexBuffer> indexBuffer;
};
