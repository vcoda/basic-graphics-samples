#include <fstream>
#include <sstream>
#include <iostream>

#include "utilities.h"
#include "magma/magma.h"

namespace utilities
{
aligned_vector<char> loadBinaryFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        const std::string msg = std::string("failed to open file \"") + filename + std::string("\"");
        throw std::runtime_error(msg.c_str());
    }
    const std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);
    aligned_vector<char> binary(static_cast<size_t>(size));
    file.read(binary.data(), size);
    file.close();
    return binary;
}

VkFormat getBlockCompressedFormat(const gliml::context& ctx)
{
    const int internalFormat = ctx.image_internal_format();
    switch (internalFormat)
    {
    case GLIML_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case GLIML_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        return VK_FORMAT_BC2_UNORM_BLOCK;
    case GLIML_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return VK_FORMAT_BC3_UNORM_BLOCK;
    default:
        throw std::invalid_argument("unknown block compressed format");
        return VK_FORMAT_UNDEFINED;
    }
}

VkFormat getSupportedDepthFormat(std::shared_ptr<magma::PhysicalDevice> physicalDevice, bool hasStencil, bool optimalTiling)
{
    const std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
    };
    for (VkFormat format : depthFormats)
    {
        if (hasStencil && !magma::Format(format).depthStencil())
            continue;
        const VkFormatProperties properties = physicalDevice->getFormatProperties(format);
        if (optimalTiling && (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
            return format;
        else if (!optimalTiling && (properties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
            return format;
    }
    return VK_FORMAT_UNDEFINED;
}

VkBool32 VKAPI_PTR reportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
    uint64_t object, size_t location, int32_t messageCode,
    const char *pLayerPrefix, const char *pMessage, void *pUserData)
{
    if (strstr(pMessage, "Extension"))
        return VK_FALSE;
    std::stringstream msg;
    msg << "[" << pLayerPrefix << "] " << pMessage << "\n";
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        std::cerr << msg.str();
    else
        std::cout << msg.str();
    return VK_FALSE;
}

} // namespace utilities
