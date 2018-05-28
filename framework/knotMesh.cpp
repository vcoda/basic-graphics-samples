#include "knotMesh.h"
#include "magma/magma.h"

KnotMesh::KnotMesh(uint32_t turns, uint32_t slices, uint32_t stacks, float radius, bool counterClockwise,
    std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    const float invSlices = 1.f / slices;
    const float invStacks = 1.f / stacks;
    std::vector<Vertex> vertices(slices * stacks);
    std::vector<rapid::vector3> ringCenters(stacks);
    // Calculate center of each ring
    for (uint32_t i = 0; i < stacks; ++i)
    {
        const float t = i * rapid::constants::two_pi * invStacks;
        float s, c;
        rapid::sincos(&s, &c, turns * t);
        float f = 1.f + 0.3f * c;
        ringCenters[i] = rapid::vector3(f * cosf(2.f * t),
                                        f * 0.3f * s,
                                        f * sinf(2.f * t));
    }
    // Loop through the rings
    for (uint32_t i = 0; i < stacks; ++i)
    {
        // Loop through the vertices making up this ring
        for(uint32_t j = 0; j < slices; ++j)
        {
            // Get the vector from the centre of this ring to the centre of the next
            rapid::vector3 tangent;
            if (i < stacks - 1)
                tangent = ringCenters[i + 1] - ringCenters[i];
            else
                tangent = ringCenters[0] - ringCenters[i];
            tangent.normalize();
            // Calculate the vector perpendicular to the tangent, 
            // pointing approximately in the positive Y direction
            const rapid::vector3 up(0.f, 1.f, 0.f);
            const rapid::vector3 side = up ^ tangent;
            const rapid::vector3 perp = (tangent ^ side).normalized() * radius;
            // Rotate about tangent vector to form the ring
            const rapid::matrix rot = rapid::rotationNormal(tangent, rapid::radians(j * 360.f * invSlices));
            const rapid::vector3 pos = ringCenters[i] + rot * perp;
            pos.store(&vertices[i * slices + j].position);
        }
    }
    std::vector<uint32_t> indices(6 * stacks * slices);
    // Loop through the rings
    for (uint32_t i = 0; i < stacks; ++i)
    {
        // Loop through the vertices making up this ring
        for (uint32_t j = 0; j < slices; ++j)
        {
            // Calculate quad indices
            uint32_t quadIndices[4];
            quadIndices[0] = i * slices + j;
            if (j != slices - 1)
                quadIndices[1] = i * slices + j + 1;
            else
                quadIndices[1] = i * slices;
            if (i != stacks - 1)
                quadIndices[2] = (i + 1) * slices + j;
            else
                quadIndices[2] = j;
            if (i != stacks - 1)
            {
                if (j != slices - 1)
                    quadIndices[3] = (i + 1) * slices + j + 1;
                else
                    quadIndices[3] = (i + 1) * slices;
            }
            else
            {
                if (j != slices - 1)
                    quadIndices[3] = j + 1;
                else
                    quadIndices[3] = 0;
            }
            const uint32_t index = (i * slices + j) * 6;
            for (int k = 0; k < 6; ++k)
            {
                static const uint32_t ccw[6] = {0, 1, 2, 2, 1, 3};
                static const uint32_t cw[6] = {0, 2, 1, 1, 2, 3};
                indices[index + k] = counterClockwise ? quadIndices[ccw[k]] 
                                                      : quadIndices[cw[k]];
            }
        }
    }
    // Clear normals
    for (auto& v : vertices)
        v.normal.x = v.normal.y = v.normal.z = 0.f;
    // Loop through the triangles
    const uint32_t numIndices = 6 * stacks * slices;
    for(uint32_t i = 0; i < numIndices; i += 3)
    {
        // Calculate normal of this triangle
        const rapid::plane trianglePlane(rapid::vector3(vertices[indices[i    ]].position), 
                                         rapid::vector3(vertices[indices[i + 1]].position), 
                                         rapid::vector3(vertices[indices[i + 2]].position));
        rapid::vector3 triNormal = trianglePlane.P;
        if (!counterClockwise)
            triNormal = -triNormal;
        for(uint32_t j = 0; j < 3; ++j)
        {
            rapid::float3& vertexNormal = vertices[indices[i + j]].normal;
            rapid::vector3 normalSum = triNormal + rapid::vector3(vertexNormal);
            normalSum.store(&vertexNormal);
        }
    }
    // Normalize all normals
    for (auto& v : vertices)
    {
        rapid::vector3 normal = rapid::vector3(v.normal).normalized();
        normal.store(&v.normal);
    }
    // Create vertex and index buffers
    vertexBuffer.reset(new magma::VertexBuffer(cmdBuffer, vertices));
    indexBuffer.reset(new magma::IndexBuffer(cmdBuffer, indices));
}

void KnotMesh::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const
{
    cmdBuffer->bindVertexBuffer(0, vertexBuffer);
    cmdBuffer->bindIndexBuffer(indexBuffer);
    cmdBuffer->drawIndexed(indexBuffer->getIndexCount(), 0, 0);
}

const magma::VertexInputState& KnotMesh::getVertexInput() const
{
    static const magma::VertexInputState vertexInput(
        magma::VertexInputBinding(0, sizeof(Vertex)),
    {
        magma::VertexInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(KnotMesh::Vertex, position)),
        magma::VertexInputAttribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(KnotMesh::Vertex, normal))
    });
    return vertexInput;
}
