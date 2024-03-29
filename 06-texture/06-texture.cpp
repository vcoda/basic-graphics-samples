#include <fstream>
#include "../framework/vulkanApp.h"
#include "../framework/bufferFromArray.h"
#include "../framework/utilities.h"

// Use Space to enable/disable multitexturing
class TextureApp : public VulkanApp
{
    struct alignas(16) UniformBlock
    {
        float lod;
        bool multitexture;
    };

    struct DescriptorSetTable : magma::DescriptorSetTable
    {
        magma::descriptor::UniformBuffer texParameters = 0;
        magma::descriptor::CombinedImageSampler diffuseImage = 1;
        magma::descriptor::CombinedImageSampler lightmapImage = 2;
        MAGMA_REFLECT(texParameters, diffuseImage, lightmapImage)
    } setTable;

    std::shared_ptr<magma::ImageView> diffuse;
    std::shared_ptr<magma::ImageView> lightmap;
    std::shared_ptr<magma::Sampler> bilinearSampler;
    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::UniformBuffer<UniformBlock>> uniformBuffer;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

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
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    virtual void onKeyDown(char key, int repeat, uint32_t flags) override
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
        magma::helpers::mapScoped(uniformBuffer,
            [this](auto *block)
            {
                block->lod = lod;
                block->multitexture = multitexture;
            });
    }

    std::shared_ptr<magma::ImageView> loadTexture(const std::string& filename, std::shared_ptr<magma::SrcTransferBuffer> buffer)
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
        magma::helpers::mapRangeScoped<uint8_t>(buffer, bufferOffset, (VkDeviceSize)size,
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
        std::vector<magma::Image::Mip> mipMaps;
        mipMaps.reserve(ctx.num_mipmaps(0));
        for (int level = 0; level < ctx.num_mipmaps(0); ++level)
        {
            magma::Image::Mip mip;
            mip.extent.width = ctx.image_width(0, level);
            mip.extent.height = ctx.image_height(0, level);
            mip.extent.depth = 1;
            mip.bufferOffset = (const uint8_t *)ctx.image_data(0, level) - firstMipData;
            mipMaps.push_back(mip);
        }
        // Upload texture data from buffer
        const magma::Image::CopyLayout bufferLayout{bufferOffset + baseMipOffset, 0, 0};
        const VkFormat format = utilities::getBlockCompressedFormat(ctx);
        std::shared_ptr<magma::Image2D> image = std::make_shared<magma::Image2D>(cmdImageCopy, format, std::move(buffer), mipMaps, bufferLayout);
        // Create image view for shader
        return std::make_shared<magma::ImageView>(std::move(image));
    }

    void loadTextures()
    {
        auto buffer = std::make_shared<magma::SrcTransferBuffer>(device, 1024 * 1024);
        cmdImageCopy->begin();
        {
            diffuse = loadTexture("brick.dds", buffer);
            lightmap = loadTexture("spot.dds", buffer);
        }
        cmdImageCopy->end();
        submitCopyImageCommands();
    }

    void createSampler()
    {
        bilinearSampler = std::make_shared<magma::Sampler>(device, magma::sampler::magMinLinearMipNearestClampToEdge);
    }

    void createVertexBuffer()
    {
        struct Vertex
        {
            rapid::float2 position;
            rapid::float2 uv;
        };

        const auto extent = diffuse->getImage()->calculateMipExtent(0);
        const float width = static_cast<float>(extent.width);
        const float height = static_cast<float>(extent.height);
        constexpr float hw = 0.5f;
        const float hh = height/width * hw;
        const std::vector<Vertex> vertices = {
            {   // top left
                {-hw, -hh},
                {0.f, 0.f},
            },
            {   // bottom left
                {-hw, hh},
                {0.f, 1.f},
            },
            {   // top right
                {hw, -hh},
                {1.f, 0.f},
            },
            {   // bottom right
                {hw, hh},
                {1.f, 1.f},
            }
        };
        vertexBuffer = vertexBufferFromArray<magma::VertexBuffer>(cmdBufferCopy, vertices);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<UniformBlock>>(device);
        updateUniforms();
    }

    void setupDescriptorSet()
    {
        setTable.texParameters = uniformBuffer;
        setTable.diffuseImage = {diffuse, bilinearSampler};
        setTable.lightmapImage = {lightmap, bilinearSampler};
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, shaderReflectionFactory, "multitexture.o");
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "passthrough.o", "multitexture.o",
            magma::renderstate::pos2fTex2f,
            magma::renderstate::triangleStrip,
            magma::renderstate::fillCullBackCcw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
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
