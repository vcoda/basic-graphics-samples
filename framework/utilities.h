#pragma once
#include <memory>
#include <vector>
#include <cassert>

#ifdef _WIN32
#undef NOMINMAX
#endif
#include <vulkan/vulkan.h>

#include "alignedAllocator.h"
#include "gliml/gliml.h"

template<typename Type>
using aligned_vector = std::vector<Type, utilities::aligned_allocator<Type>>;

namespace magma
{
    class PhysicalDevice;
}

namespace utilities
{
    aligned_vector<char> loadBinaryFile(const std::string& filename);

    VkFormat getBlockCompressedFormat(const gliml::context& ctx);
    VkFormat getSupportedDepthFormat(std::shared_ptr<magma::PhysicalDevice> physicalDevice,
        bool hasStencil, bool optimalTiling);
    uint32_t getSupportedMultisampleLevel(std::shared_ptr<magma::PhysicalDevice> physicalDevice,
        VkFormat format);

    VkBool32 VKAPI_PTR reportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
        uint64_t object, size_t location, int32_t messageCode,
        const char *pLayerPrefix, const char *pMessage, void *pUserData);

    template<class Vertex, std::size_t Size>
    inline std::unique_ptr<magma::VertexBuffer> makeVertexBuffer(const Vertex (&vertices)[Size],
        magma::lent_ptr<magma::CommandBuffer> cmdBuffer,
        std::unique_ptr<magma::Allocator> allocator = nullptr)
    {
        static_assert(Size > 0, "invalid vertex array size");
        auto vertexBuffer = std::make_unique<magma::VertexBuffer>(std::move(cmdBuffer),
            Size * sizeof(Vertex), vertices, std::move(allocator));
        vertexBuffer->setVertexCount(static_cast<uint32_t>(Size));
        return vertexBuffer;
    }

    template<class Vertex>
    inline std::unique_ptr<magma::VertexBuffer> makeVertexBuffer(const std::vector<Vertex>& vertices,
        magma::lent_ptr<magma::CommandBuffer> cmdBuffer,
        std::unique_ptr<magma::Allocator> allocator = nullptr)
    {
        MAGMA_ASSERT(!vertices.empty());
        auto vertexBuffer = std::make_unique<magma::VertexBuffer>(std::move(cmdBuffer),
            vertices.size() * sizeof(Vertex), vertices.data(), std::move(allocator));
        vertexBuffer->setVertexCount(magma::core::countof(vertices));
        return vertexBuffer;
    }
} // namespace utilities
