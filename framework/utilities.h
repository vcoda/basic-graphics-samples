#pragma once
#include <memory>
#include <vector>
#include <cassert>

#ifdef _WIN32
#define NOMINMAX
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
} // namespace utilities
