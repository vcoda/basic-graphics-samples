#pragma once
#include "mesh.h"

class PlaneMesh : public Mesh
{
public:
    struct Vertex
    {
        rapid::float3 position;
        rapid::float3 normal;
        rapid::float2 texCoord;
    };

public:
    PlaneMesh(float width, float height, bool twoSided,
        std::shared_ptr<magma::CommandBuffer> cmdBuffer);
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const override;
    virtual const magma::VertexInputState& getVertexInput() const override;

private:
    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::VertexBuffer> normalBuffer;
    std::shared_ptr<magma::VertexBuffer> texCoordBuffer;
    std::shared_ptr<magma::IndexBuffer> indexBuffer;
};
