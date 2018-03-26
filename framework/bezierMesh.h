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

private:
    struct Patch
    {
        Patch(const uint32_t numFaces, 
            const std::vector<uint32_t>& faceIndices, 
            const std::vector<rapid::float3>& vertices, 
            const std::vector<rapid::float3>& normals,
            const std::vector<rapid::float2>& texCoords,
            std::shared_ptr<magma::CommandBuffer> cmdBuffer);

        std::shared_ptr<magma::VertexBuffer> vertexBuffer;
        std::shared_ptr<magma::VertexBuffer> normalBuffer;
        std::shared_ptr<magma::VertexBuffer> texCoordBuffer;
        std::shared_ptr<magma::IndexBuffer> indexBuffer;
    };

    std::vector<std::shared_ptr<Patch>> patches;
};
