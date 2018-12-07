#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"

// Use Space to enable/disable multitexturing
class TextureApp : public VulkanApp
{
    struct Texture
    {
        std::shared_ptr<magma::Image2D> image;
        std::shared_ptr<magma::ImageView> imageView;
    };

    struct alignas(16) UniformBlock
    {
        bool multitexture;
    };

    Texture diffuse;
    Texture lightmap;

    std::shared_ptr<magma::Sampler> bilinearSampler;
    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::UniformBuffer<UniformBlock>> uniformBuffer;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

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
        submitCmdBuffer(bufferIndex);
    }

    virtual void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case AppKey::Space:
            multitexture = !multitexture;
            updateUniform();
            break;
        }
        VulkanApp::onKeyDown(key, repeat, flags);
    }

    void updateUniform()
    {
        magma::helpers::mapScoped<UniformBlock>(uniformBuffer, true, [this](auto *block)
        {
            block->multitexture = multitexture;
        });
    }

    Texture loadTexture(const std::string& filename)
    {
        utilities::aligned_vector<char> buffer = utilities::loadBinaryFile(filename);
        gliml::context ctx;
        ctx.enable_dxt(true);
        if (!ctx.load(buffer.data(), static_cast<unsigned>(buffer.size())))
            throw std::runtime_error("failed to load DDS texture");
        // Setup texture data description
        const VkFormat format = utilities::getBCFormat(ctx);
        std::vector<const void *> mipData;
        std::vector<VkDeviceSize> mipSizes;
        for (int level = 0; level < ctx.num_mipmaps(0); ++level)
        {
            const void *imageData = ctx.image_data(0, level);
            MAGMA_ASSERT(MAGMA_ALIGNED(imageData));
            mipData.push_back(imageData);
            mipSizes.push_back(ctx.image_size(0, level));
        }
        // Upload texture data
        const VkExtent2D extent = {ctx.image_width(0, 0), ctx.image_height(0, 0)};
        Texture texture;
        texture.image = std::make_shared<magma::Image2D>(device, format, extent, mipData, mipSizes, cmdImageCopy);
        // Create image view for shader
        texture.imageView = std::make_shared<magma::ImageView>(texture.image);
        return texture;
    }

    void createSampler()
    {
        bilinearSampler = std::make_shared<magma::Sampler>(device, magma::samplers::linearMipmapNearestClampToEdge);
    }

    void createVertexBuffer()
    {
        struct Vertex
        {
            rapid::float2 position;
            rapid::float2 uv;
        };
        const auto extent = diffuse.image->getMipExtent(0);
        const float width = static_cast<float>(extent.width);
        const float height = static_cast<float>(extent.height);
        const float hw = 0.5f;
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
        vertexBuffer = std::make_shared<magma::VertexBuffer>(device, vertices);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<UniformBlock>>(device);
        updateUniform();
    }

    void setupDescriptorSet()
    {   // Create descriptor pool
        constexpr uint32_t maxDescriptorSets = 1; // One set is enough for us
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        const magma::Descriptor imageSamplerDesc = magma::descriptors::CombinedImageSampler(1);
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, maxDescriptorSets,
            std::vector<magma::Descriptor>
            {
                uniformBufferDesc, // Allocate one uniform buffer
                magma::descriptors::CombinedImageSampler(2) // Allocate two combined image samplers
            });
        // Setup descriptor set layout
        descriptorSetLayout = std::make_shared<magma::DescriptorSetLayout>(device,
            std::initializer_list<magma::DescriptorSetLayout::Binding>
            {
                magma::bindings::FragmentStageBinding(0, uniformBufferDesc), // Bind uniform buffer to slot 0 in fragment shader
                magma::bindings::FragmentStageBinding(1, imageSamplerDesc),  // Bind diffuse sampler to slot 1 in fragment shader
                magma::bindings::FragmentStageBinding(2, imageSamplerDesc)   // Bind lightmap sampler to slot 2 in fragment shader
            });
        // Allocate and update descriptor set
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformBuffer);
        descriptorSet->update(1, diffuse.imageView, bilinearSampler);
        descriptorSet->update(2, lightmap.imageView, bilinearSampler);
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
        graphicsPipeline = std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            std::vector<magma::PipelineShaderStage>
            {
                VertexShader(device, "passthrough.o"),
                FragmentShader(device, "multitexture.o")
            },
            magma::renderstates::pos2FloatTex2Float,
            magma::renderstates::triangleStrip,
            magma::renderstates::fillCullBackCCW,
            magma::renderstates::noMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::dontBlendWriteRGB,
            std::initializer_list<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->setRenderArea(0, 0, width, height);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clears::grayColor});
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(pipelineLayout, descriptorSet);
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
    return std::make_unique<TextureApp>(entry);
}
