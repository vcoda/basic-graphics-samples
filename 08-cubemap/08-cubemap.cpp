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
    std::shared_ptr<magma::GraphicsPipeline> wireframeDrawPipeline;

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
        diffuse = loadDDSCubeMap("diff.dds");
        specular = loadDDSCubeMap("spec.dds");
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
        submitCmdBuffer(bufferIndex);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 8.f);
        const rapid::vector3 center(0.f, 0.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        const float fov = rapid::radians(60.f);
        const float aspect = width/(float)height;
        const float zn = 1.f, zf = 100.f;
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
        const uint32_t subdivisionDegree = 16;
        mesh = std::make_unique<BezierPatchMesh>(teapotPatches, kTeapotNumPatches, teapotVertices, subdivisionDegree, cmdBufferCopy);
    }

    TextureCube loadDDSCubeMap(const std::string& filename)
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
        const VkFormat format = utilities::getBCFormat(ctx);
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
        anisotropicSampler = std::make_shared<magma::Sampler>(device, magma::samplers::anisotropicClampToEdge);
    }

    void createUniformBuffer()
    {
        uniformTransforms = std::make_shared<magma::UniformBuffer<TransformMatrices>>(device);
    }

    void setupDescriptorSet()
    {
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        const magma::Descriptor imageSamplerDesc = magma::descriptors::CombinedImageSampler(1);
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, 1,
            std::vector<magma::Descriptor>
            {
                uniformBufferDesc,
                magma::descriptors::CombinedImageSampler(2)
            });
        descriptorSetLayout = std::make_shared<magma::DescriptorSetLayout>(device,
            std::initializer_list<magma::DescriptorSetLayout::Binding>
            {
                magma::bindings::VertexStageBinding(0, uniformBufferDesc),
                magma::bindings::FragmentStageBinding(1, imageSamplerDesc),
                magma::bindings::FragmentStageBinding(2, imageSamplerDesc)
            });
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformTransforms);
        descriptorSet->update(1, diffuse.imageView, anisotropicSampler);
        descriptorSet->update(2, specular.imageView, anisotropicSampler);
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
        wireframeDrawPipeline = std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            std::vector<magma::PipelineShaderStage>
            {
                VertexShader(device, "transform.o"),
                FragmentShader(device, "envmap.o")
            },
            mesh->getVertexInput(),
            magma::renderstates::triangleList,
            negateViewport ? magma::renderstates::fillCullBackCW : magma::renderstates::fillCullBackCCW,
            magma::renderstates::noMultisample,
            magma::renderstates::depthLessOrEqual,
            magma::renderstates::dontBlendWriteRGB,
            std::initializer_list<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->setRenderArea(0, 0, width, height);
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
                cmdBuffer->bindDescriptorSet(pipelineLayout, descriptorSet);
                cmdBuffer->bindPipeline(wireframeDrawPipeline);
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
