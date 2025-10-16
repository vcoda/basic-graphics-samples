#include <fstream>
#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"

/* Place all texture data (with mipmaps) into one large
   source transfer buffer and copy all textures to device
   using single vkQueueSubmit() call. */
#define BATCH_LOAD

// Use Space to enable/disable multitexturing
class TextureApp : public VulkanApp
{
    struct alignas(16) UniformBlock
    {
        float lod;
        bool multitexture;
    };

    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer texParameters = 0;
        magma::descriptor::CombinedImageSampler diffuseImage = 1;
        magma::descriptor::CombinedImageSampler lightmapImage = 2;
    } setTable;

    std::unique_ptr<magma::ImageView> diffuse;
    std::unique_ptr<magma::ImageView> lightmap;
    std::unique_ptr<magma::Sampler> bilinearSampler;
    std::unique_ptr<magma::VertexBuffer> vertexBuffer;
    std::unique_ptr<magma::UniformBuffer<UniformBlock>> uniformBuffer;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

    float lod = 0.f;
    bool multitexture = true;

public:
    TextureApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("06 - Texture"), 512, 512)
    {
        initialize();
        loadTextures();
        createSampler();
        createVertexBuffer();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
    }

    void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case AppKey::PgUp:
            if (lod < diffuse->getImage()->getMipLevels() - 1)
            {
                lod += 1.f;
                updateUniforms();
            }
            break;
        case AppKey::PgDn:
            if (lod > 0.f)
            {
                lod -= 1.f;
                updateUniforms();
            }
            break;
        case AppKey::Space:
            multitexture = !multitexture;
            updateUniforms();
            break;
        }
        VulkanApp::onKeyDown(key, repeat, flags);
    }

    void updateUniforms()
    {
        magma::map(uniformBuffer,
            [this](auto *block)
            {
                block->lod = lod;
                block->multitexture = multitexture;
            });
    }

    std::unique_ptr<magma::ImageView> loadTextureBatch(const std::string& filename, const std::unique_ptr<magma::SrcTransferBuffer>& buffer)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        const std::streamoff size = file.tellg();
        file.seekg(0, std::ios::beg);
        gliml::context ctx;
        ctx.enable_dxt(true);
        VkDeviceSize bufferOffset = buffer->getPrivateData();
        VkDeviceSize baseMipOffset = 0;
        magma::mapRange<uint8_t>(buffer, bufferOffset, (VkDeviceSize)size,
            [&](uint8_t *data)
            {   // Read data to buffer
                file.read(reinterpret_cast<char *>(data), size);
                file.close();
                if (!ctx.load(data, static_cast<unsigned>(size)))
                    throw std::runtime_error("failed to load DDS texture");
                // Skip DDS header
                baseMipOffset = reinterpret_cast<const uint8_t *>(ctx.image_data(0, 0)) - data;
            });
        buffer->setPrivateData(bufferOffset + size);
        // Setup texture data description
        const uint8_t *firstMipData = (const uint8_t *)ctx.image_data(0, 0);
        magma::Mipmap mipMap;
        mipMap.reserve(ctx.num_mipmaps(0));
        for (int level = 0; level < ctx.num_mipmaps(0); ++level)
        {
            const ptrdiff_t offset = (const uint8_t *)ctx.image_data(0, level) - firstMipData;
            mipMap.emplace_back(
                ctx.image_width(0, level),
                ctx.image_height(0, level),
                (VkDeviceSize)offset);
        }
        // Upload texture data from buffer
        const magma::Image::CopyLayout bufferLayout{bufferOffset + baseMipOffset, 0, 0};
        const VkFormat format = utilities::getBlockCompressedFormat(ctx);
        std::unique_ptr<magma::Image> image = std::make_unique<magma::Image2D>(cmdImageCopy, format, std::move(buffer), mipMap, bufferLayout);
        // Create image view for shader
        return std::make_unique<magma::UniqueImageView>(std::move(image));
    }

    std::unique_ptr<magma::ImageView> loadTextureFromData(const std::string& filename)
    {   // Simple, but suboptimal way to load texture from host memory
        auto buffer = utilities::loadBinaryFile(filename);
        gliml::context ctx;
        ctx.enable_dxt(true);
        if (!ctx.load(buffer.data(), static_cast<unsigned>(buffer.size())))
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        // Setup texture data description
        const VkFormat format = utilities::getBlockCompressedFormat(ctx);
        magma::Mipmap mipMap;
        mipMap.reserve(ctx.num_mipmaps(0));
        for (int level = 0; level < ctx.num_mipmaps(0); ++level)
        {
             mipMap.emplace_back(
                ctx.image_width(0, level),
                ctx.image_height(0, level),
                ctx.image_size(0, level),
                ctx.image_data(0, level));
        }
        // Upload texture from host memory
        std::unique_ptr<magma::Image> image = std::make_unique<magma::Image2D>(cmdImageCopy, format, mipMap,
            nullptr, magma::Image::Initializer{}, magma::Sharing(), memcpy);
        // Create image view for shader
        return std::make_unique<magma::UniqueImageView>(std::move(image));
    }

    void loadTextures()
    {
    #ifdef BATCH_LOAD
        constexpr VkDeviceSize bufferSize = 1024 * 1024;
        auto buffer = std::make_unique<magma::SrcTransferBuffer>(device, bufferSize);
        cmdImageCopy->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
            diffuse = loadTextureBatch("brick.dds", buffer);
            lightmap = loadTextureBatch("spot.dds", buffer);
        }
        cmdImageCopy->end();
        submitCopyImageCommands();
    #else
        diffuse = loadTextureFromData("brick.dds");
        lightmap = loadTextureFromData("spot.dds");
    #endif // BATCH_LOAD
    }

    void createSampler()
    {
        bilinearSampler = std::make_unique<magma::Sampler>(device, magma::sampler::magMinLinearMipNearestClampToEdge);
    }

    void createVertexBuffer()
    {
        const float width = static_cast<float>(diffuse->getImage()->getWidth());
        const float height = static_cast<float>(diffuse->getImage()->getHeight());
        constexpr float w = 0.5f;
        const float h = height/width * w; // Keep aspect ratio
        const magma::vt::Pos2fTex2f vertices[] = {
            {-w, -h, 0.f, 0.f},
            {-w,  h, 0.f, 1.f},
            { w, -h, 1.f, 0.f},
            { w,  h, 1.f, 1.f}
        };
        vertexBuffer = utilities::makeVertexBuffer(vertices, cmdBufferCopy);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_unique<magma::UniformBuffer<UniformBlock>>(device);
        updateUniforms();
    }

    void setupDescriptorSet()
    {
        setTable.texParameters = uniformBuffer;
        setTable.diffuseImage = {diffuse, bilinearSampler};
        setTable.lightmapImage = {lightmap, bilinearSampler};
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, 0, shaderReflectionFactory, "multitexture");
    }

    void setupPipeline()
    {
        std::unique_ptr<magma::PipelineLayout> layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            "passthrough", "multitexture",
            magma::renderstate::pos2fTex2f,
            magma::renderstate::triangleStrip,
            magma::renderstate::fillCullBackCcw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            std::move(layout),
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clear::gray});
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, 0, descriptorSet);
                cmdBuffer->bindPipeline(graphicsPipeline);
                cmdBuffer->bindVertexBuffer(0, vertexBuffer);
                cmdBuffer->draw(4, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<TextureApp>(new TextureApp(entry));
}
