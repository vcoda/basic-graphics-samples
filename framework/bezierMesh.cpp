#include "bezierMesh.h"
#include "bezier.inl"
#include "magma/magma.h"
#include "rapid/rapid.h"

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
    const uint32_t vertexCount = (divs + 1) * (divs + 1);
    std::shared_ptr<magma::SrcTransferBuffer> vertices(std::make_shared<magma::SrcTransferBuffer>(
        cmdBuffer->getDevice(), vertexCount * sizeof(rapid::float3)));
    std::shared_ptr<magma::SrcTransferBuffer> normals(std::make_shared<magma::SrcTransferBuffer>(
        cmdBuffer->getDevice(), vertexCount * sizeof(rapid::float3)));
    std::shared_ptr<magma::SrcTransferBuffer> texCoords(std::make_shared<magma::SrcTransferBuffer>(
        cmdBuffer->getDevice(), vertexCount * sizeof(rapid::float2)));
    rapid::vector3 controlPoints[16];
    for (uint32_t np = 0; np < numPatches; ++np)
    {   // Set patch control points
        for (uint32_t i = 0; i < 16; ++i)
        {
            controlPoints[i] = rapid::vector3(patchVertices[patches[np][i] - 1][0],
                                              patchVertices[patches[np][i] - 1][1],
                                              patchVertices[patches[np][i] - 1][2]);
        }
        rapid::float3 *P = static_cast<rapid::float3 *>(vertices->getMemory()->map());
        rapid::float3 *N = static_cast<rapid::float3 *>(normals->getMemory()->map());
        rapid::float2 *st = static_cast<rapid::float2 *>(texCoords->getMemory()->map());
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
        for (uint32_t i = 0; i < vertexCount; ++i)
        {   // Swap Y and Z component to match coordinate system
            std::swap(P[i].y, P[i].z);
            std::swap(N[i].y, N[i].z);
        }
        texCoords->getMemory()->unmap();
        normals->getMemory()->unmap();
        vertices->getMemory()->unmap();
        // Triangulate and load to buffers
        std::shared_ptr<Patch> patch(std::make_shared<Patch>(cmdBuffer, vertices, normals, texCoords));
        this->patches.push_back(patch);
    }
    const uint32_t numFaces = divs * divs;
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
    std::shared_ptr<magma::SrcTransferBuffer> srcBuffer(std::make_shared<magma::SrcTransferBuffer>(
        cmdBuffer->getDevice(), numFaces * 2 * 3 * sizeof(uint32_t)));
    magma::helpers::mapScoped<uint32_t>(srcBuffer, [numFaces, indices](uint32_t *faces)
    {
        for (uint32_t i = 0, k = 0, n = 0; i < numFaces; ++i, k += 4) // For each face
        {
            for (uint32_t j = 0; j < 2; ++j) // For each triangle in the face
            {
                faces[n    ] = indices[k];
                faces[n + 1] = indices[k + j + 1];
                faces[n + 2] = indices[k + j + 2];
                n += 3;
            }
        }
    });
    indexBuffer = std::make_shared<magma::IndexBuffer>(cmdBuffer, srcBuffer, VK_INDEX_TYPE_UINT32);
}

void BezierPatchMesh::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const
{
    cmdBuffer->bindIndexBuffer(indexBuffer);
    for (const auto& patch : patches)
    {
        cmdBuffer->bindVertexBuffer(0, patch->vertexBuffer);
        cmdBuffer->bindVertexBuffer(1, patch->normalBuffer);
        cmdBuffer->bindVertexBuffer(2, patch->texCoordBuffer);
        cmdBuffer->drawIndexed(indexBuffer->getIndexCount(), 0, 0);
    }
}

const magma::VertexInputState& BezierPatchMesh::getVertexInput() const
{
    static constexpr magma::VertexInputBinding bindings[] = {
        magma::VertexInputBinding(0, sizeof(rapid::float3)), // Position
        magma::VertexInputBinding(1, sizeof(rapid::float3)), // Normal
        magma::VertexInputBinding(2, sizeof(rapid::float2))  // TexCoord
    };
    static constexpr magma::VertexInputAttribute attributes[] = {
        magma::VertexInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
        magma::VertexInputAttribute(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),
        magma::VertexInputAttribute(2, 2, VK_FORMAT_R32G32_SFLOAT, 0)
    };
    static constexpr magma::VertexInputState vertexInput(bindings, attributes);
    return vertexInput;
}

BezierPatchMesh::Patch::Patch(std::shared_ptr<magma::CommandBuffer> cmdBuffer,
    std::shared_ptr<magma::SrcTransferBuffer> vertices,
    std::shared_ptr<magma::SrcTransferBuffer> normals,
    std::shared_ptr<magma::SrcTransferBuffer> texCoords)
{
    vertexBuffer = std::make_shared<magma::VertexBuffer>(cmdBuffer, vertices);
    normalBuffer = std::make_shared<magma::VertexBuffer>(cmdBuffer, normals);
    texCoordBuffer = std::make_shared<magma::VertexBuffer>(cmdBuffer, texCoords);
}
