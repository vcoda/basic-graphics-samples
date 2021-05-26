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

    struct SetLayout : public magma::DescriptorSetDeclaration
    {
        magma::binding::UniformBuffer transforms = 0;
        magma::binding::CombinedImageSampler diffuse = 1;
        magma::binding::CombinedImageSampler specular = 2;
        MAGMA_REFLECT(&transforms, &diffuse, &specular)
    } setLayout;

    std::unique_ptr<quadric::Teapot> mesh;
    std::shared_ptr<magma::ImageView> diffuse;
    std::shared_ptr<magma::ImageView> specular;
    std::shared_ptr<magma::Sampler> anisotropicSampler;
    std::shared_ptr<magma::UniformBuffer<TransformMatrices>> uniformTransforms;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    rapid::matrix view;
    rapid::matrix proj;

public:
    TextureCubeApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("08 - Cubemap texture"), 512, 512, true)
    {
        initialize();
        setupView();
        createMesh();
        diffuse = loadCubeMap("diff.dds");
        specular = loadCubeMap("spec.dds");
        createSampler();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
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
        magma::helpers::mapScoped(uniformTransforms,
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

    std::shared_ptr<magma::ImageView> loadCubeMap(const std::string& filename)
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
        {   // Read data from file
            file.read(reinterpret_cast<char *>(data), size);
            ctx.enable_dxt(true);
            if (!ctx.load(data, static_cast<unsigned>(size)))
                throw std::runtime_error("failed to load DDS texture");
            // Skip DDS header
            baseMipOffset = reinterpret_cast<const uint8_t *>(ctx.image_data(0, 0)) - data;
        });
        // Setup texture data description
        const VkFormat format = utilities::getBlockCompressedFormat(ctx);
        const uint32_t dimension = ctx.image_width(0, 0);
        magma::Image::MipmapLayout mipOffsets;
        VkDeviceSize lastMipSize = 0;
        for (int face = 0; face < ctx.num_faces(); ++face)
        {
            mipOffsets.push_back(lastMipSize);
            const int mipLevels = ctx.num_mipmaps(face);
            for (int level = 1; level < mipLevels; ++level)
            {   // Compute relative offset
                const intptr_t mipOffset = (const uint8_t *)ctx.image_data(face, level) - (const uint8_t *)ctx.image_data(face, level - 1);
                mipOffsets.push_back(mipOffset);
            }
            lastMipSize = ctx.image_size(face, mipLevels - 1);
        }
        // Upload texture data from buffer
        magma::Image::CopyLayout bufferLayout{baseMipOffset, 0, 0};
        std::shared_ptr<magma::ImageCube> image = std::make_shared<magma::ImageCube>(cmdImageCopy, format, dimension, ctx.num_mipmaps(0), buffer, mipOffsets, bufferLayout);
        // Create image view for fragment shader
        return std::make_shared<magma::ImageView>(std::move(image));
    }

    void createSampler()
    {
        anisotropicSampler = std::make_shared<magma::Sampler>(device, magma::sampler::magMinLinearMipAnisotropicClampToEdge);
    }

    void createUniformBuffer()
    {
        uniformTransforms = std::make_shared<magma::UniformBuffer<TransformMatrices>>(device);
    }

    void setupDescriptorSet()
    {
        setLayout.transforms = uniformTransforms;
        setLayout.diffuse = {diffuse, anisotropicSampler};
        setLayout.specular = {specular, anisotropicSampler};
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            0, setLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, shaderReflectionFactory, "envmap.o");
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "transform.o", "envmap.o",
            mesh->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::fillCullBackCCW : magma::renderstate::fillCullBackCW,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLessOrEqual,
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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clears::grayColor,
                    magma::clears::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, descriptorSet);
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
