#include <vector>
#include "planeMesh.h"
#include "magma/magma.h"
#include "rapid/rapid.h"

PlaneMesh::PlaneMesh(float width, float height, bool textured, bool twoSided, std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    const float hw = width *.5f;
    const float hh = height * .5f;
    const std::vector<rapid::float3> vertices = {
        {-hw,-hh, 0.f},
        {-hw, hh, 0.f},
        { hw,-hh, 0.f},
        { hw, hh, 0.f},
    };
    vertexBuffer.reset(new magma::VertexBuffer(cmdBuffer, vertices));
    const std::vector<rapid::float3> normals(6, rapid::float3(0.f, 0.f, 1.f));
    normalBuffer.reset(new magma::VertexBuffer(cmdBuffer, normals));
    if (textured)
    {
        const std::vector<rapid::float2> texCoords = {
            {0.f, 0.f},
            {0.f, 1.f},
            {1.f, 0.f},
            {1.f, 1.f}
        };
        texCoordBuffer.reset(new magma::VertexBuffer(cmdBuffer, texCoords));
    }
    const std::vector<uint16_t> indices = twoSided ?
        std::vector<uint16_t>{0, 1, 2, 2, 1, 3, 0, 2, 1, 1, 2, 3} :
        std::vector<uint16_t>{0, 1, 2, 2, 1, 3};
    indexBuffer.reset(new magma::IndexBuffer(cmdBuffer, indices));
}

void PlaneMesh::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const
{
    cmdBuffer->bindVertexBuffer(0, vertexBuffer);
    cmdBuffer->bindVertexBuffer(1, normalBuffer);
    if (texCoordBuffer)
        cmdBuffer->bindVertexBuffer(2, texCoordBuffer);
    cmdBuffer->bindIndexBuffer(indexBuffer);
    cmdBuffer->drawIndexed(indexBuffer->getIndexCount(), 0, 0);
}

const magma::VertexInputState& PlaneMesh::getVertexInput() const
{
    static const magma::VertexInputState vertexInput(
    {
        magma::VertexInputBinding(0, sizeof(rapid::float3)), // Position
        magma::VertexInputBinding(1, sizeof(rapid::float3)), // Normal
        magma::VertexInputBinding(2, sizeof(rapid::float2))  // TexCoord
    },
    {
        magma::VertexInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
        magma::VertexInputAttribute(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),
        magma::VertexInputAttribute(2, 2, VK_FORMAT_R32G32_SFLOAT, 0)
    });
    return vertexInput;
}
