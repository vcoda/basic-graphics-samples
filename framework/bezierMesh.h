#pragma once
#include <vector>
#include "mesh.h"
#include "rapid/rapid.h"

// https://www.scratchapixel.com/lessons/advanced-rendering/bezier-curve-rendering-utah-teapot
class BezierPatchMesh : public Mesh
{
public:
    BezierPatchMesh(const uint32_t patches[][16], 
        const uint32_t numPatches, 
        const float patchVertices[][3],
        const uint32_t subdivisionDegree, 
        std::shared_ptr<magma::CommandBuffer> cmdBuffer);
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const override;
    virtual const magma::VertexInputState& getVertexInput() const override;

private:
    struct Patch
    {
        Patch(std::shared_ptr<magma::CommandBuffer> cmdBuffer,
            std::shared_ptr<magma::SrcTransferBuffer> vertices,
            std::shared_ptr<magma::SrcTransferBuffer> normals,
            std::shared_ptr<magma::SrcTransferBuffer> texCoords);

        std::shared_ptr<magma::VertexBuffer> vertexBuffer;
        std::shared_ptr<magma::VertexBuffer> normalBuffer;
        std::shared_ptr<magma::VertexBuffer> texCoordBuffer;
    };

    std::vector<std::shared_ptr<Patch>> patches;
    std::shared_ptr<magma::IndexBuffer> indexBuffer;
};
