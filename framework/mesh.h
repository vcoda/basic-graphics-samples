#pragma once
#include <memory>
#include "magma/magma.h"
#include "rapid/rapid.h"

class Mesh : public magma::core::NonCopyable
{
public:
    virtual void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer) const = 0;
    virtual const magma::VertexInputState& getVertexInput() const = 0;
};

namespace magma
{
    namespace specialization
    {
        template<> struct VertexAttribute<rapid::float2> : public AttributeFormat<VK_FORMAT_R32G32_SFLOAT> {};
        template<> struct VertexAttribute<rapid::float3> : public AttributeFormat<VK_FORMAT_R32G32B32_SFLOAT> {};
        template<> struct VertexAttribute<rapid::float4> : public AttributeFormat<VK_FORMAT_R32G32B32A32_SFLOAT> {};
    } // namespace specialization
} // namespace magma
