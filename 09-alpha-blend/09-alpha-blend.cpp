#include "../framework/vulkanApp.h"
#include "../framework/cubeMesh.h"
#include "../framework/utilities.h"

class AlphaBlendApp : public VulkanApp
{
    std::unique_ptr<CubeMesh> mesh;
    std::shared_ptr<magma::Image2D> image;
    std::shared_ptr<magma::ImageView> imageView;
    std::shared_ptr<magma::Sampler> anisotropicSampler;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformWorldViewProj;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> cullFrontPipeline;
    std::shared_ptr<magma::GraphicsPipeline> cullBackPipeline;

    rapid::matrix viewProj;
    bool negateViewport = false;

public:
    AlphaBlendApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("09 - Alpha blend"), 512, 512)
    {
        initialize();
        negateViewport = extensions->KHR_maintenance1 || extensions->AMD_negative_viewport_height;
        setupView();
        createMesh();
        loadTexture("logo.dds");
        createSampler();
        createUniformBuffers();
        setupDescriptorSet();
        cullFrontPipeline = setupPipeline(negateViewport ? magma::renderstates::fillCullFrontCCW : magma::renderstates::fillCullFrontCW);
        cullBackPipeline = setupPipeline(negateViewport ? magma::renderstates::fillCullBackCCW : magma::renderstates::fillCullBackCW);
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        timer->run();
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCommandBuffer(bufferIndex);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 4.f);
        const rapid::vector3 center(0.f, 0.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        constexpr float fov = rapid::radians(60.f);
        const float aspect = width/(float)height;
        constexpr float zn = 1.f, zf = 100.f;
        const rapid::matrix view = rapid::lookAtRH(eye, center, up);
        const rapid::matrix proj = rapid::perspectiveFovRH(fov, aspect, zn, zf);
        viewProj = view * proj;
    }

    void updatePerspectiveTransform()
    {
        constexpr float speed = 0.05f;
        static float angle = 0.f;
        angle += timer->millisecondsElapsed() * speed;
        const float radians = rapid::radians(angle);
        const rapid::matrix pitch = rapid::rotationX(radians);
        const rapid::matrix yaw = rapid::rotationY(radians);
        const rapid::matrix roll = rapid::rotationZ(radians);
        const rapid::matrix world = pitch * yaw * roll;
        magma::helpers::mapScoped<rapid::matrix>(uniformWorldViewProj, true, [this, &world](auto *worldViewProj)
        {
            *worldViewProj = world * viewProj;
        });
    }

    void createMesh()
    {
        mesh = std::make_unique<CubeMesh>(cmdBufferCopy);
    }

    void loadTexture(const std::string& filename)
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
        image = std::make_shared<magma::Image2D>(device, format, extent, mipData, mipSizes, cmdImageCopy);
        // Create image view for shader
        imageView = std::make_shared<magma::ImageView>(image);
    }

    void createSampler()
    {
        anisotropicSampler = std::make_shared<magma::Sampler>(device, magma::samplers::magMinLinearMipAnisotropicClampToEdge);
    }

    void createUniformBuffers()
    {
        uniformWorldViewProj = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void setupDescriptorSet()
    {
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        const magma::Descriptor imageSamplerDesc = magma::descriptors::CombinedImageSampler(1);
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, 1,
            std::vector<magma::Descriptor>
            {
                uniformBufferDesc,
                imageSamplerDesc
            });
        descriptorSetLayout = std::make_shared<magma::DescriptorSetLayout>(device,
            std::initializer_list<magma::DescriptorSetLayout::Binding>
            {
                magma::bindings::VertexStageBinding(0, uniformBufferDesc),
                magma::bindings::FragmentStageBinding(1, imageSamplerDesc)
            });
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformWorldViewProj);
        descriptorSet->update(1, imageView, anisotropicSampler);
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
    }

    std::shared_ptr<magma::GraphicsPipeline> setupPipeline(const magma::RasterizationState& rasterizationState) const
    {
        return std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            std::vector<magma::PipelineShaderStage>
            {
                VertexShader(device, "transform.o"),
                FragmentShader(device, "texture.o")
            },
            mesh->getVertexInput(),
            magma::renderstates::triangleStrip,
            rasterizationState,
            magma::renderstates::noMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::blendNormalWriteRgb,
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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::ClearColor(0.35f, 0.53f, 0.7f, 1.0f)});
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(pipelineLayout, descriptorSet);
                // Draw back faced triangles
                cmdBuffer->bindPipeline(cullFrontPipeline);
                mesh->draw(cmdBuffer);
                // Draw front facing triangles
                cmdBuffer->bindPipeline(cullBackPipeline);
                mesh->draw(cmdBuffer);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::make_unique<AlphaBlendApp>(entry);
}
