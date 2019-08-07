#pragma once
#include <vector>
#include <cassert>
#include "alignedAllocator.h"
#include <vulkan/vulkan.h>
#include "gliml/gliml.h"

namespace utilities
{
    template<typename Type>
    using aligned_vector = std::vector<Type, aligned_allocator<Type>>;

    aligned_vector<char> loadBinaryFile(const std::string& filename);
    VkFormat getBCFormat(const gliml::context& ctx);
} // namespace utilities
