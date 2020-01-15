#include "shapeMesh.h"

PlaneMesh::PlaneMesh(float width, float height, bool twoSided,
    std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    const float hw = width * 0.5f;
    const float hh = height * 0.5f;
    const rapid::float3 z(0.f, 0.f, 1.f);
    const std::vector<Vertex> vertices = {
        {
            {-hw,-hh, 0.f},
            z,
            {0.f, 0.f}
        },
        {
            {-hw, hh, 0.f},
            z,
            {0.f, 1.f},
        },
        {
            {hw,-hh, 0.f},
            z,
            {1.f, 0.f},
        },
        {
            {hw, hh, 0.f},
            z,
            {1.f, 1.f}
        }
    };
    vertexBuffer = std::make_shared<magma::VertexBuffer>(cmdBuffer, vertices);
    indexBuffer = std::make_shared<magma::IndexBuffer>(cmdBuffer, twoSided ?
        std::vector<uint16_t>{0, 1, 2, 2, 1, 3, 0, 2, 1, 1, 2, 3} :
        std::vector<uint16_t>{0, 1, 2, 2, 1, 3});
}

void PlaneMesh::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const
{
    cmdBuffer->bindVertexBuffer(0, vertexBuffer);
    cmdBuffer->bindIndexBuffer(indexBuffer);
    cmdBuffer->drawIndexed(indexBuffer->getIndexCount(), 0, 0);
}

const magma::VertexInputState& PlaneMesh::getVertexInput() const
{
    static magma::VertexInputStructure<Vertex> vertexInput(0, {
        {0, &Vertex::position},
        {1, &Vertex::normal},
        {2, &Vertex::texCoord}});
    return vertexInput;
}

CubeMesh::CubeMesh(std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    const rapid::float3 positions[] = {
        // back
        {-1.f, 1.f,-1.f},
        { 1.f, 1.f,-1.f},
        {-1.f,-1.f,-1.f},
        { 1.f,-1.f,-1.f},
        // front
        {-1.f, 1.f, 1.f},
        {-1.f,-1.f, 1.f},
        { 1.f, 1.f, 1.f},
        { 1.f,-1.f, 1.f},
        // top
        {-1.f, 1.f, 1.f},
        { 1.f, 1.f, 1.f},
        {-1.f, 1.f,-1.f},
        { 1.f, 1.f,-1.f},
        // bottom
        {-1.f,-1.f, 1.f},
        {-1.f,-1.f,-1.f},
        { 1.f,-1.f, 1.f},
        { 1.f,-1.f,-1.f},
        // right
        { 1.f, 1.f,-1.f},
        { 1.f, 1.f, 1.f},
        { 1.f,-1.f,-1.f},
        { 1.f,-1.f, 1.f},
        // left
        {-1.f, 1.f,-1.f},
        {-1.f,-1.f,-1.f},
        {-1.f, 1.f, 1.f},
        {-1.f,-1.f, 1.f}
    };

    // .z stores face index
    const rapid::float3 texCoords[] = {
        // back
        {1.f, 0.f, 0.f},
        {0.f, 0.f, 0.f},
        {1.f, 1.f, 0.f},
        {0.f, 1.f, 0.f},
        // front
        {0.f, 0.f, 1.f},
        {0.f, 1.f, 1.f},
        {1.f, 0.f, 1.f},
        {1.f, 1.f, 1.f},
        // top
        {1.f, 0.f, 2.f},
        {0.f, 0.f, 2.f},
        {1.f, 1.f, 2.f},
        {0.f, 1.f, 2.f},
        // bottom
        {1.f, 0.f, 3.f},
        {0.f, 0.f, 3.f},
        {1.f, 1.f, 3.f},
        {0.f, 1.f, 3.f},
        // right
        {1.f, 0.f, 4.f},
        {0.f, 0.f, 4.f},
        {1.f, 1.f, 4.f},
        {0.f, 1.f, 4.f},
        // left
        {0.f, 0.f, 5.f},
        {0.f, 1.f, 5.f},
        {1.f, 0.f, 5.f},
        {1.f, 1.f, 5.f}
    };

    std::vector<Vertex> vertices(24);
    for (int i = 0; i < 24; ++i)
        vertices[i] = {positions[i], texCoords[i]};
    vertexBuffer = std::make_shared<magma::VertexBuffer>(cmdBuffer, vertices);
}

void CubeMesh::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const
{
    cmdBuffer->bindVertexBuffer(0, vertexBuffer);
    for (uint32_t firstVertex = 0; firstVertex <= 20; firstVertex += 4)
        cmdBuffer->draw(4, firstVertex);
}

const magma::VertexInputState& CubeMesh::getVertexInput() const
{
    static magma::VertexInputStructure<Vertex> vertexInput(0, {
        {0, &Vertex::position},
        {1, &Vertex::texCoord}});
    return vertexInput;
}

