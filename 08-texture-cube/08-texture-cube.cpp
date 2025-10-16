#include <fstream>
#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"
#include "quadric/include/teapot.h"

// Use L button + mouse to rotate scene
class TextureCubeApp : public VulkanApp
{
    struct TransformMatrices
    {
        rapid::matrix worldView;
        rapid::matrix worldViewProj;
        rapid::matrix normal;
    };

    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer transforms = 0;
        magma::descriptor::CombinedImageSampler diffuse = 1;
        magma::descriptor::CombinedImageSampler specular = 2;
    } setTable;

    std::unique_ptr<quadric::Teapot> mesh;
    std::unique_ptr<magma::ImageView> diffuse;
    std::unique_ptr<magma::ImageView> specular;
    std::unique_ptr<magma::Sampler> anisotropicSampler;
    std::unique_ptr<magma::UniformBuffer<TransformMatrices>> uniformTransforms;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

    rapid::matrix view;
    rapid::matrix proj;

public:
    TextureCubeApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("08 - Cubemap texture"), 512, 512, true)
    {
        initialize();
        setupView();
        createMesh();
        loadCubeMaps();
        createSampler();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
    }

    void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCommandBuffer(bufferIndex);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 8.f);
        const rapid::vector3 center(0.f, 0.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        constexpr float fov = rapid::radians(60.f);
        const float aspect = width/(float)height;
        constexpr float zn = 1.f, zf = 100.f;
        view = rapid::lookAtRH(eye, center, up);
        proj = rapid::perspectiveFovRH(fov, aspect, zn, zf);
    }

    void updatePerspectiveTransform()
    {
        const rapid::matrix pitch = rapid::rotationX(rapid::radians(spinY/2.f));
        const rapid::matrix yaw = rapid::rotationY(rapid::radians(spinX/2.f));
        const rapid::matrix trans = rapid::translation(0.f, -1.25f, 0.f);
        const rapid::matrix world = trans * pitch * yaw;
        magma::map(uniformTransforms,
            [this, &world](auto *block)
            {
                block->worldView = world * view;
                block->worldViewProj = block->worldView * proj;
                block->normal = rapid::transpose(rapid::inverse(block->worldView));
            });
    }

    void createMesh()
    {
        constexpr uint16_t subdivisionDegree = 16;
        mesh = std::make_unique<quadric::Teapot>(subdivisionDegree, cmdBufferCopy);
    }

    std::unique_ptr<magma::ImageView> loadCubeMap(const std::string& filename, const std::unique_ptr<magma::SrcTransferBuffer>& buffer)
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
            {   // Read data from file
                file.read(reinterpret_cast<char *>(data), size);
                if (!ctx.load(data, static_cast<unsigned>(size)))
                    throw std::runtime_error("failed to load DDS texture");
                // Skip DDS header
                baseMipOffset = reinterpret_cast<const uint8_t *>(ctx.image_data(0, 0)) - data;
            });
        buffer->setPrivateData(bufferOffset + size);
        // Setup texture data description
        const uint32_t dimension = ctx.image_width(0, 0);
        const uint8_t *firstMipData = (const uint8_t *)ctx.image_data(0, 0);
        magma::Mipmap mipMap;
        mipMap.reserve(ctx.num_faces() * ctx.num_mipmaps(0));
        for (int face = 0; face < ctx.num_faces(); ++face)
        {
            for (int level = 0; level < ctx.num_mipmaps(face); ++level)
            {
                const ptrdiff_t offset = (const uint8_t *)ctx.image_data(face, level) - firstMipData;
                mipMap.emplace_back(dimension, dimension, (VkDeviceSize)offset);
            }
        }
        // Upload texture data from buffer
        const magma::Image::CopyLayout bufferLayout{bufferOffset + baseMipOffset, 0, 0};
        const VkFormat format = utilities::getBlockCompressedFormat(ctx);
        auto image = std::make_unique<magma::ImageCube>(cmdImageCopy,
            format, std::move(buffer), mipMap, bufferLayout);
        // Create image view for fragment shader
        return std::make_unique<magma::UniqueImageView>(std::move(image));
    }

    void loadCubeMaps()
    {
        constexpr VkDeviceSize bufferSize = 1024 * 1024;
        auto buffer = std::make_unique<magma::SrcTransferBuffer>(device, bufferSize);
        cmdImageCopy->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
            diffuse = loadCubeMap("diff.dds", buffer);
            specular = loadCubeMap("spec.dds", buffer);
        }
        cmdImageCopy->end();
        submitCopyImageCommands();
    }

    void createSampler()
    {
        anisotropicSampler = std::make_unique<magma::Sampler>(device, magma::sampler::magMinLinearMipAnisotropicClampToEdge8x);
    }

    void createUniformBuffer()
    {
        uniformTransforms = std::make_unique<magma::UniformBuffer<TransformMatrices>>(device);
    }

    void setupDescriptorSet()
    {
        setTable.transforms = uniformTransforms;
        setTable.diffuse = {diffuse, anisotropicSampler};
        setTable.specular = {specular, anisotropicSampler};
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, 0, shaderReflectionFactory, "envmap");
    }

    void setupPipeline()
    {
        auto layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            "transform", "envmap",
            mesh->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::fillCullBackCcw
                           : magma::renderstate::fillCullBackCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLessOrEqual,
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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clear::gray,
                    magma::clear::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -int32_t(height) : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, 0, descriptorSet);
                cmdBuffer->bindPipeline(graphicsPipeline);
                mesh->draw(cmdBuffer);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<TextureCubeApp>(new TextureCubeApp(entry));
}
