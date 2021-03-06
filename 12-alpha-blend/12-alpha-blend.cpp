#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"
#include "quadric/include/cube.h"

class AlphaBlendApp : public VulkanApp
{
    struct SetLayout : public magma::DescriptorSetDeclaration
    {
        magma::binding::UniformBuffer worldViewProj = 0;
        magma::binding::CombinedImageSampler diffuse = 1;
        MAGMA_REFLECT(&worldViewProj, &diffuse)
    } setLayout;

    std::unique_ptr<quadric::Cube> mesh;
    std::shared_ptr<magma::Image2D> image;
    std::shared_ptr<magma::ImageView> imageView;
    std::shared_ptr<magma::Sampler> anisotropicSampler;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformWorldViewProj;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> cullFrontPipeline;
    std::shared_ptr<magma::GraphicsPipeline> cullBackPipeline;

    rapid::matrix viewProj;

public:
    AlphaBlendApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("12 - Alpha blending"), 512, 512)
    {
        initialize();
        setupView();
        createMesh();
        loadTexture("logo.dds");
        createUniformBuffers();
        createSampler();
        setupDescriptorSet();
        cullFrontPipeline = setupPipeline(negateViewport ? magma::renderstate::fillCullFrontCCW : magma::renderstate::fillCullFrontCW);
        cullBackPipeline = setupPipeline(negateViewport ? magma::renderstate::fillCullBackCCW : magma::renderstate::fillCullBackCW);
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
        magma::helpers::mapScoped(uniformWorldViewProj,
            [this, &world](auto *worldViewProj)
            {
                *worldViewProj = world * viewProj;
            });
    }

    void createMesh()
    {
        mesh = std::make_unique<quadric::Cube>(cmdBufferCopy);
    }

    void loadTexture(const std::string& filename)
    {
        aligned_vector<char> buffer = utilities::loadBinaryFile(filename);
        gliml::context ctx;
        ctx.enable_dxt(true);
        if (!ctx.load(buffer.data(), static_cast<unsigned>(buffer.size())))
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        // Setup texture data description
        const VkFormat format = utilities::getBlockCompressedFormat(ctx);
        magma::Image::MipmapData mipData;
        magma::Image::MipmapLayout mipSizes;
        for (int level = 0; level < ctx.num_mipmaps(0); ++level)
        {
            const void *imageData = ctx.image_data(0, level);
            MAGMA_ASSERT(MAGMA_ALIGNED(imageData));
            mipData.push_back(imageData);
            mipSizes.push_back(ctx.image_size(0, level));
        }
        // Upload texture data
        const VkExtent2D extent = {ctx.image_width(0, 0), ctx.image_height(0, 0)};
        image = std::make_shared<magma::Image2D>(cmdImageCopy, format, extent, mipData, mipSizes);
        // Create image view for shader
        imageView = std::make_shared<magma::ImageView>(image);
    }

    void createUniformBuffers()
    {
        uniformWorldViewProj = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void createSampler()
    {
        anisotropicSampler = std::make_shared<magma::Sampler>(device, magma::sampler::magMinLinearMipAnisotropicClampToEdge);
    }

    void setupDescriptorSet()
    {
        setLayout.worldViewProj = uniformWorldViewProj;
        setLayout.diffuse = {imageView, anisotropicSampler};
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, shaderReflectionFactory, "texture.o");
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
    }

    std::shared_ptr<magma::GraphicsPipeline> setupPipeline(const magma::RasterizationState& rasterizationState) const
    {
        return std::make_shared<GraphicsPipeline>(device,
            "transform.o", "texture.o",
            mesh->getVertexInput(),
            magma::renderstate::triangleList,
            rasterizationState,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::blendNormalRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::ClearColor(0.35f, 0.53f, 0.7f, 1.0f)});
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(cullFrontPipeline, 0, descriptorSet);
                // Draw back faced triangles
                cmdBuffer->bindPipeline(cullFrontPipeline);
                mesh->draw(cmdBuffer);
                // Draw front faced triangles
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
    return std::unique_ptr<AlphaBlendApp>(new AlphaBlendApp(entry));
}
