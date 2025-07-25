#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"
#include "quadric/include/cube.h"

class AlphaBlendApp : public VulkanApp
{
    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer worldViewProj = 0;
        magma::descriptor::CombinedImageSampler diffuse = 1;
    } setTable;

    std::unique_ptr<quadric::Cube> mesh;
    std::unique_ptr<magma::ImageView> logo;
    std::unique_ptr<magma::Sampler> anisotropicSampler;
    std::unique_ptr<magma::UniformBuffer<rapid::matrix>> uniformWorldViewProj;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> cullFrontPipeline;
    std::unique_ptr<magma::GraphicsPipeline> cullBackPipeline;

    rapid::matrix viewProj;

public:
    AlphaBlendApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("12 - Alpha blending"), 512, 512)
    {
        initialize();
        setupView();
        createMesh();
        loadTextures();
        createUniformBuffers();
        createSampler();
        setupDescriptorSet();
        cullFrontPipeline = setupPipeline(negateViewport ? magma::renderstate::fillCullFrontCcw : magma::renderstate::fillCullFrontCw);
        cullBackPipeline = setupPipeline(negateViewport ? magma::renderstate::fillCullBackCcw : magma::renderstate::fillCullBackCw);
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
        timer->run();
    }

    void render(uint32_t bufferIndex) override
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
        magma::map(uniformWorldViewProj,
            [this, &world](auto *worldViewProj)
            {
                *worldViewProj = world * viewProj;
            });
    }

    void createMesh()
    {
        mesh = std::make_unique<quadric::Cube>(cmdBufferCopy);
    }

    std::unique_ptr<magma::ImageView> loadTexture(const std::string& filename, const std::unique_ptr<magma::SrcTransferBuffer>& buffer)
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
        std::unique_ptr<magma::Image> image = std::make_unique<magma::Image2D>(cmdImageCopy, format, std::move(buffer), mipMaps, bufferLayout);
        // Create image view for shader
        return std::make_unique<magma::UniqueImageView>(std::move(image));
    }

    void loadTextures()
    {
        constexpr VkDeviceSize bufferSize = 1024 * 1024;
        auto buffer = std::make_unique<magma::SrcTransferBuffer>(device, bufferSize);
        cmdImageCopy->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
            logo = loadTexture("logo.dds", buffer);
        }
        cmdImageCopy->end();
        submitCopyImageCommands();
    }

    void createUniformBuffers()
    {
        uniformWorldViewProj = std::make_unique<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void createSampler()
    {
        anisotropicSampler = std::make_unique<magma::Sampler>(device, magma::sampler::magMinLinearMipAnisotropicClampToEdge8x);
    }

    void setupDescriptorSet()
    {
        setTable.worldViewProj = uniformWorldViewProj;
        setTable.diffuse = {logo, anisotropicSampler};
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, 0, shaderReflectionFactory, "texture");
    }

    std::unique_ptr<magma::GraphicsPipeline> setupPipeline(const magma::RasterizationState& rasterizationState) const
    {
        std::unique_ptr<magma::PipelineLayout> layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout());
        return std::make_unique<GraphicsPipeline>(device,
            "transform", "texture",
            mesh->getVertexInput(),
            magma::renderstate::triangleList,
            rasterizationState,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::blendNormalRgb,
            std::move(layout),
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::ClearColor(0.35f, 0.53f, 0.7f, 1.f)
                });
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
