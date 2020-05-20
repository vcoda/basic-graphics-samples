#include <fstream>
#include "../framework/vulkanApp.h"
#include "../framework/bezierMesh.h"
#include "../framework/utilities.h"
#include "teapot.h"

// Use L button + mouse to rotate scene
class CubeMapApp : public VulkanApp
{
    struct TextureCube
    {
        std::shared_ptr<magma::ImageCube> image;
        std::shared_ptr<magma::ImageView> imageView;
    };

    struct TransformMatrices
    {
        rapid::matrix worldView;
        rapid::matrix worldViewProj;
        rapid::matrix normal;
    };

    std::unique_ptr<BezierPatchMesh> mesh;
    TextureCube diffuse;
    TextureCube specular;

    std::shared_ptr<magma::Sampler> anisotropicSampler;
    std::shared_ptr<magma::UniformBuffer<TransformMatrices>> uniformTransforms;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    rapid::matrix view;
    rapid::matrix proj;
    bool negateViewport = false;

public:
    CubeMapApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("08 - CubeMap"), 512, 512, true)
    {
        initialize();
        negateViewport = extensions->KHR_maintenance1 || extensions->AMD_negative_viewport_height;
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
        magma::helpers::mapScoped<TransformMatrices>(uniformTransforms, true, [this, &world](auto *block)
        {
            block->worldView = world * view;
            block->worldViewProj = block->worldView * proj;
            block->normal = rapid::transpose(rapid::inverse(block->worldView));
        });
    }

    void createMesh()
    {
        constexpr uint32_t subdivisionDegree = 16;
        mesh = std::make_unique<BezierPatchMesh>(teapotPatches, kTeapotNumPatches, teapotVertices, subdivisionDegree, cmdBufferCopy);
    }

    TextureCube loadCubeMap(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        const std::streamoff size = file.tellg();
        file.seekg(0, std::ios::beg);
        gliml::context ctx;
        VkDeviceSize baseMipOffset = 0;
        // Allocate transfer buffer
        std::shared_ptr<magma::Buffer> buffer = std::make_shared<magma::SrcTransferBuffer>(device, size);
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
        magma::ImageMipmapLayout mipOffsets;
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
        auto image = std::make_shared<magma::ImageCube>(device, format, dimension, ctx.num_mipmaps(0), buffer, baseMipOffset, mipOffsets, cmdImageCopy);
        // Create image view for fragment shader
        auto imageView = std::make_shared<magma::ImageView>(image);
        return TextureCube{image, imageView};
    }

    void createSampler()
    {
        anisotropicSampler = std::make_shared<magma::Sampler>(device, magma::samplers::magMinLinearMipAnisotropicClampToEdge);
    }

    void createUniformBuffer()
    {
        uniformTransforms = std::make_shared<magma::UniformBuffer<TransformMatrices>>(device);
    }

    void setupDescriptorSet()
    {
        constexpr magma::Descriptor oneUniformBuffer = magma::descriptors::UniformBuffer(1);
        constexpr magma::Descriptor oneImageSampler = magma::descriptors::CombinedImageSampler(1);
        // Create descriptor pool
        constexpr uint32_t maxDescriptorSets = 1;
        descriptorPool = std::shared_ptr<magma::DescriptorPool>(new magma::DescriptorPool(device, maxDescriptorSets,
            {
                oneUniformBuffer,
                magma::descriptors::CombinedImageSampler(2) // Allocate two combined image samplers
            }));
        // Setup descriptor set layout
        descriptorSetLayout = std::shared_ptr<magma::DescriptorSetLayout>(new magma::DescriptorSetLayout(device,
            {
                magma::bindings::VertexStageBinding(0, oneUniformBuffer),  // Bind transforms to slot 0 in the vertex shader
                magma::bindings::FragmentStageBinding(1, oneImageSampler), // Bind diffuse cubemap to slot 1 in the fragment shader
                magma::bindings::FragmentStageBinding(2, oneImageSampler)  // Bind specular cubemap to slot 2 in the fragment shader
            }));
        // Allocate and update descriptor set
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformTransforms);
        descriptorSet->update(1, diffuse.imageView, anisotropicSampler);
        descriptorSet->update(2, specular.imageView, anisotropicSampler);
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "transform.o", "envmap.o",
            mesh->getVertexInput(),
            magma::renderstates::triangleList,
            negateViewport ? magma::renderstates::fillCullBackCW : magma::renderstates::fillCullBackCCW,
            magma::renderstates::dontMultisample,
            magma::renderstates::depthLessOrEqual,
            magma::renderstates::dontBlendRgb,
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
    return std::make_unique<CubeMapApp>(entry);
}
