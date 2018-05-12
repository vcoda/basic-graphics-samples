#include <fstream>
#include "utilities.h"
#include "magma/shared.h"

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
    return std::move(binary);
}

VkFormat getBCFormat(const gliml::context& ctx)
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
    }
    return VK_FORMAT_UNDEFINED;
}
} // namespace utilities
