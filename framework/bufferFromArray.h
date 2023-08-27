#pragma once

template<typename BufferType, typename VertexType, uint32_t vertexCount>
inline std::shared_ptr<BufferType> vertexBufferFromArray(std::shared_ptr<magma::CommandBuffer> cmdBuffer,
    const VertexType (&vertices)[vertexCount],
    std::shared_ptr<magma::Allocator> allocator = nullptr,
    const magma::Sharing& sharing = magma::Sharing(),
    magma::CopyMemoryFunction copyFn = nullptr)
{
    static_assert(vertexCount > 0, "number of vertices should be greater than 0");
    auto vertexBuffer = std::make_shared<BufferType>(std::move(cmdBuffer),
        static_cast<VkDeviceSize>(sizeof(VertexType) * vertexCount), vertices,
        std::move(allocator), magma::Buffer::Descriptor(), sharing, std::move(copyFn));
    vertexBuffer->setVertexCount(vertexCount);
    return vertexBuffer;
}

template<typename BufferType, typename VertexType>
inline std::shared_ptr<BufferType> vertexBufferFromArray(std::shared_ptr<magma::CommandBuffer> cmdBuffer,
    const std::vector<VertexType>& vertices,
    std::shared_ptr<magma::Allocator> allocator = nullptr,
    const magma::Sharing& sharing = magma::Sharing(),
    magma::CopyMemoryFunction copyFn = nullptr)
{
    auto vertexBuffer = std::make_shared<BufferType>(std::move(cmdBuffer),
        static_cast<VkDeviceSize>(sizeof(VertexType) * vertices.size()), vertices.data(),
        std::move(allocator), magma::Buffer::Descriptor(), sharing, std::move(copyFn));
    vertexBuffer->setVertexCount(MAGMA_COUNT(vertices));
    return vertexBuffer;
}

template<typename IndexType, uint32_t indexCount>
inline std::shared_ptr<magma::IndexBuffer> indexBufferFromArray(std::shared_ptr<magma::CommandBuffer> cmdBuffer,
    const IndexType (&indices)[indexCount],
    std::shared_ptr<magma::Allocator> allocator = nullptr,
    const magma::Sharing& sharing = magma::Sharing(),
    magma::CopyMemoryFunction copyFn = nullptr)
{
    static_assert(std::is_same<IndexType, uint16_t>::value ||
        std::is_same<IndexType, uint32_t>::value,
        "element type of index buffer should be unsigned short or unsigned int");
    return std::make_shared<magma::IndexBuffer>(std::move(cmdBuffer),
        static_cast<VkDeviceSize>(sizeof(IndexType) * indexCount), indices,
        std::is_same<IndexType, uint16_t>::value ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32,
        std::move(allocator), magma::Buffer::Descriptor(), sharing, std::move(copyFn));
}

template<typename IndexType>
inline std::shared_ptr<magma::IndexBuffer> indexBufferFromArray(std::shared_ptr<magma::CommandBuffer> cmdBuffer,
    const std::vector<IndexType>& indices,
    std::shared_ptr<magma::Allocator> allocator = nullptr,
    const magma::Sharing& sharing = magma::Sharing(),
    magma::CopyMemoryFunction copyFn = nullptr)
{
    static_assert(std::is_same<IndexType, uint16_t>::value ||
        std::is_same<IndexType, uint32_t>::value,
        "element type of index buffer should be unsigned short or unsigned int");
    return std::make_shared<magma::IndexBuffer>(std::move(cmdBuffer),
        static_cast<VkDeviceSize>(sizeof(IndexType) * indices.size()), indices.data(),
        std::is_same<IndexType, uint16_t>::value ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32,
        std::move(allocator), magma::Buffer::Descriptor(), sharing, std::move(copyFn));
}
