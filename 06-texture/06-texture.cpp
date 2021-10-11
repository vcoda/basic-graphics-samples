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

    struct SetLayout : magma::DescriptorSetDeclaration
    {
        magma::binding::UniformBuffer texParameters = 0;
        magma::binding::CombinedImageSampler diffuseImage = 1;
        magma::binding::CombinedImageSampler lightmapImage = 2;
        MAGMA_REFLECT(&texParameters, &diffuseImage, &lightmapImage)
    } setLayout;

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
        diffuse = loadTexture("brick.dds");
        lightmap = loadTexture("spot.dds");
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

    std::shared_ptr<magma::ImageView> loadTexture(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        const std::streamoff size = file.tellg();
        file.seekg(0, std::ios::beg);
        gliml::context ctx;
        VkDeviceSize baseMipOffset = 0;
        std::shared_ptr<magma::SrcTransferBuffer> buffer = std::make_shared<magma::SrcTransferBuffer>(device, size);
        magma::helpers::mapScoped<uint8_t>(buffer, [&](uint8_t *data)
        {   // Read data to buffer
            file.read(reinterpret_cast<char *>(data), size);
            file.close();
            ctx.enable_dxt(true);
            if (!ctx.load(data, static_cast<unsigned>(size)))
                throw std::runtime_error("failed to load DDS texture");
            // Skip DDS header
            baseMipOffset = reinterpret_cast<const uint8_t *>(ctx.image_data(0, 0)) - data;
        });
        // Setup texture data description
        const VkFormat format = utilities::getBlockCompressedFormat(ctx);
        const VkExtent2D extent = {ctx.image_width(0, 0), ctx.image_height(0, 0)};
        magma::Image::MipmapLayout mipOffsets(1, 0);
        for (int level = 1; level < ctx.num_mipmaps(0); ++level)
        {   // Compute relative offset
            const intptr_t mipOffset = (const uint8_t *)ctx.image_data(0, level) - (const uint8_t *)ctx.image_data(0, level - 1);
            mipOffsets.push_back(mipOffset);
        }
        // Upload texture data from buffer
        magma::Image::CopyLayout bufferLayout{baseMipOffset, 0, 0};
        std::shared_ptr<magma::Image2D> image = std::make_shared<magma::Image2D>(cmdImageCopy, format, extent, buffer, mipOffsets, bufferLayout);
        // Create image view for shader
        return std::make_shared<magma::ImageView>(std::move(image));
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

        const auto extent = diffuse->getImage()->getMipExtent(0);
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
        setLayout.texParameters = uniformBuffer;
        setLayout.diffuseImage = {diffuse, bilinearSampler};
        setLayout.lightmapImage = {lightmap, bilinearSampler};
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, shaderReflectionFactory, "multitexture.o");
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "passthrough.o", "multitexture.o",
            magma::renderstate::pos2fTex2f,
            magma::renderstate::triangleStrip,
            magma::renderstate::fillCullBackCCW,
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
