#include <vector>
#include "cubeMesh.h"
#include "magma/magma.h"
#include "rapid/rapid.h"

CubeMesh::CubeMesh(std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    const std::vector<rapid::float3> vertices = {
        // back
        {-1.0f, 1.0f,-1.0f},
        {1.0f, 1.0f,-1.0f},
        {-1.0f,-1.0f,-1.0f},
        {1.0f,-1.0f,-1.0f},
        // front
        {-1.0f, 1.0f, 1.0f},
        {-1.0f,-1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f,-1.0f, 1.0f},
        // top
        {-1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f,-1.0f},
        {1.0f, 1.0f,-1.0f},
        // bottom
        {-1.0f,-1.0f, 1.0f},
        {-1.0f,-1.0f,-1.0f},
        {1.0f,-1.0f, 1.0f},
        {1.0f,-1.0f,-1.0f},
        // right
        {1.0f, 1.0f,-1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f,-1.0f,-1.0f},
        {1.0f,-1.0f, 1.0f},
        // left
        {-1.0f, 1.0f,-1.0f},
        {-1.0f,-1.0f,-1.0f},
        {-1.0f, 1.0f, 1.0f},
        {-1.0f,-1.0f, 1.0f}
    };

    // .z stores face index
    const std::vector<rapid::float3> texCoords = {
        // back
        {1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        // front
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        // top
        {1.0f, 0.0f, 2.0f},
        {0.0f, 0.0f, 2.0f},
        {1.0f, 1.0f, 2.0f},
        {0.0f, 1.0f, 2.0f},
        // bottom
        {1.0f, 0.0f, 3.0f},
        {0.0f, 0.0f, 3.0f},
        {1.0f, 1.0f, 3.0f},
        {0.0f, 1.0f, 3.0f},
        // right
        {1.0f, 0.0f, 4.0f},
        {0.0f, 0.0f, 4.0f},
        {1.0f, 1.0f, 4.0f},
        {0.0f, 1.0f, 4.0f},
        // left
        {0.0f, 0.0f, 5.0f},
        {0.0f, 1.0f, 5.0f},
        {1.0f, 0.0f, 5.0f},
        {1.0f, 1.0f, 5.0f}
    };

    vertexBuffer = std::make_shared<magma::VertexBuffer>(cmdBuffer, vertices);
    texCoordBuffer = std::make_shared<magma::VertexBuffer>(cmdBuffer, texCoords);
}

void CubeMesh::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const
{
    cmdBuffer->bindVertexBuffer(0, vertexBuffer);
    cmdBuffer->bindVertexBuffer(1, texCoordBuffer);
    for (uint32_t firstVertex = 0; firstVertex <= 20; firstVertex += 4)
        cmdBuffer->draw(4, firstVertex);
}

const magma::VertexInputState& CubeMesh::getVertexInput() const
{
    static constexpr magma::VertexInputBinding bindings[] = {
        magma::VertexInputBinding(0, sizeof(rapid::float3)), // Position
        magma::VertexInputBinding(1, sizeof(rapid::float3))  // TexCoord
    };
    static constexpr magma::VertexInputAttribute attributes[] = {
        magma::VertexInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
        magma::VertexInputAttribute(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0)
    };
    static constexpr magma::VertexInputState vertexInput(bindings, attributes);
    return vertexInput;
}
