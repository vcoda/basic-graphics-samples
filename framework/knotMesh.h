#pragma once
#include "mesh.h"
#include "rapid/rapid.h"

class KnotMesh : public Mesh
{
public:
    struct Vertex
    {
        rapid::float3 position;
        rapid::float3 normal;
    };

public:
    KnotMesh(uint32_t turns, uint32_t slices, uint32_t stacks, float radius, bool counterClockwise,
        std::shared_ptr<magma::CommandBuffer> cmdBuffer);
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const override;
    virtual const magma::VertexInputState& getVertexInput() const override;

private:
    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::IndexBuffer> indexBuffer;
};
