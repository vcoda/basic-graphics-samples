#include "bezierMesh.h"
#include "bezier.inl"
#include "magma/magma.h"

BezierPatchMesh::BezierPatchMesh(
    const uint32_t patches[][16], 
    const uint32_t numPatches, 
    const float patchVertices[][3],
    const uint32_t subdivisionDegree, 
    std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    assert(subdivisionDegree >= 2);
    assert(subdivisionDegree <= 32);
    const uint32_t divs = subdivisionDegree;
    uint32_t numFaces = divs * divs;
    std::vector<uint32_t> indices(numFaces * 4);
    // All patches are subdivided in the same way, so here we share the same topology
    for (uint16_t j = 0, k = 0; j < divs; ++j) 
    {
        for (uint16_t i = 0; i < divs; ++i, ++k) 
        {
            indices[k * 4] = (divs + 1) * j + i;
            indices[k * 4 + 1] = (divs + 1) * j + i + 1;
            indices[k * 4 + 2] = (divs + 1) * (j + 1) + i + 1;
            indices[k * 4 + 3] = (divs + 1) * (j + 1) + i;
        }
    }
    std::vector<rapid::float3> P((divs + 1) * (divs + 1));
    std::vector<rapid::float3> N((divs + 1) * (divs + 1));
    std::vector<rapid::float2> st((divs + 1) * (divs + 1));
    rapid::vector3 controlPoints[16];
    for (uint32_t np = 0; np < numPatches; ++np)
    {
        // Set patch control points
        for (uint32_t i = 0; i < 16; ++i)
        {
            controlPoints[i] = rapid::vector3(patchVertices[patches[np][i] - 1][0],
                                              patchVertices[patches[np][i] - 1][1],
                                              patchVertices[patches[np][i] - 1][2]);
        }
        // Generate grid
        for (uint16_t j = 0, k = 0; j <= divs; ++j)
        {
            float v = j / (float)divs;
            for (uint16_t i = 0; i <= divs; ++i, ++k)
            {
                float u = i / (float)divs;
                evalBezierPatch(controlPoints, u, v).store(&P[k]);
                rapid::vector3 dU = dUBezier(controlPoints, u, v);
                rapid::vector3 dV = dVBezier(controlPoints, u, v);
                rapid::vector3 normal = (dU^dV).normalized();
                normal.store(&N[k]);
                st[k].x = u;
                st[k].y = v;
            }
        }
        // Swap Y and Z component to match coordinate system
        for (auto& v : P) std::swap(v.y, v.z);
        for (auto& n : N) std::swap(n.y, n.z);
        // Triangulate and load to buffers
        std::shared_ptr<Patch> patch(new Patch(numFaces, indices, P, N, st, cmdBuffer));
        this->patches.push_back(patch);
    }
}

void BezierPatchMesh::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const
{
    for (const auto& patch : patches)
    {
        cmdBuffer->bindVertexBuffer(0, patch->vertexBuffer);
        cmdBuffer->bindVertexBuffer(1, patch->normalBuffer);
        cmdBuffer->bindVertexBuffer(2, patch->texCoordBuffer);
        cmdBuffer->bindIndexBuffer(patch->indexBuffer);
        cmdBuffer->drawIndexed(patch->indexBuffer->getIndexCount(), 0, 0);
    }
}

BezierPatchMesh::Patch::Patch(const uint32_t numFaces, 
    const std::vector<uint32_t>& faceIndices, 
    const std::vector<rapid::float3>& vertices,
    const std::vector<rapid::float3>& normals,
    const std::vector<rapid::float2>& texCoords,
    std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    std::vector<uint32_t> indices(numFaces * 2 * 3);
    for (uint32_t i = 0, k = 0, n = 0; i < numFaces; ++i, k += 4) // For each face
    { 
        for (uint32_t j = 0; j < 2; ++j) // For each triangle in the face
        { 
            indices[n    ] = faceIndices[k];
            indices[n + 1] = faceIndices[k + j + 1];
            indices[n + 2] = faceIndices[k + j + 2];
            n += 3;
        }
    }
    vertexBuffer.reset(new magma::VertexBuffer(cmdBuffer, vertices));
    normalBuffer.reset(new magma::VertexBuffer(cmdBuffer, normals));
    texCoordBuffer.reset(new magma::VertexBuffer(cmdBuffer, texCoords));
    indexBuffer.reset(new magma::IndexBuffer(cmdBuffer, indices));
}
