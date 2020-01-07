#include "planeMesh.h"

PlaneMesh::PlaneMesh(float width, float height, bool twoSided,
    std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    const float hw = width *.5f;
    const float hh = height * .5f;
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
    const std::vector<uint16_t> indices = twoSided ?
        std::vector<uint16_t>{0, 1, 2, 2, 1, 3, 0, 2, 1, 1, 2, 3} :
        std::vector<uint16_t>{0, 1, 2, 2, 1, 3};
    indexBuffer = std::make_shared<magma::IndexBuffer>(cmdBuffer, indices);
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
