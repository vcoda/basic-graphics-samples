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
    std::unique_ptr<TextureCube> diffuse;
    std::unique_ptr<TextureCube> specular;
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
        queue->submit(
            commandBuffers[bufferIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished,
            renderFinished,
            waitFences[bufferIndex]);
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
        mesh.reset(new BezierPatchMesh(teapotPatches, kTeapotNumPatches, teapotVertices, subdivisionDegree, cmdBufferCopy));
    }

    std::unique_ptr<TextureCube> loadCubeMap(const std::string& filename)
    {
        utilities::aligned_vector<char> buffer = utilities::loadBinaryFile(filename);
        gliml::context ctx;
        ctx.enable_dxt(true);
        if (!ctx.load(buffer.data(), static_cast<unsigned>(buffer.size())))
            throw std::runtime_error("failed to load DDS texture");
        if (ctx.texture_target() != GLIML_GL_TEXTURE_CUBE_MAP)
            throw std::runtime_error("not a cubemap texture");

        const VkFormat format = utilities::getBCFormat(ctx);
        std::vector<uint32_t> mipDimensions;
        std::vector<VkDeviceSize> mipSizes;
        for (int level = 0; level < ctx.num_mipmaps(0); ++level)
        {
            mipDimensions.push_back(static_cast<uint32_t>(ctx.image_width(0, level)));
            mipSizes.push_back(ctx.image_size(0, level));
        }
        std::vector<const void *> cubeMipData[6];
        for (int face = 0; face < ctx.num_faces(); ++face)
        {
            for (int level = 0; level < ctx.num_mipmaps(face); ++level)
            {
                const void *imageData = ctx.image_data(face, level);
                MAGMA_ASSERT(MAGMA_ALIGNED(imageData));
                cubeMipData[face].push_back(imageData);
            }
        }

        std::unique_ptr<TextureCube> texture(new TextureCube);
        texture->image.reset(new magma::ImageCube(device, format, mipDimensions, cubeMipData, mipSizes, cmdImageCopy));
        texture->imageView.reset(new magma::ImageView(texture->image));
        return texture;
    }

    void createSampler()
    {
        anisotropicSampler.reset(new magma::Sampler(device, magma::samplers::anisotropicClampToEdge));
    }

    void createUniformBuffer()
    {
        uniformTransforms.reset(new magma::UniformBuffer<TransformMatrices>(device));
    }

    void setupDescriptorSet()
    {
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        const magma::Descriptor imageSamplerDesc = magma::descriptors::CombinedImageSampler(1);
        descriptorPool.reset(new magma::DescriptorPool(device, 1, {
            uniformBufferDesc,
            magma::descriptors::CombinedImageSampler(2)
        }));
        descriptorSetLayout.reset(new magma::DescriptorSetLayout(device, {
            magma::bindings::VertexStageBinding(0, uniformBufferDesc),
            magma::bindings::FragmentStageBinding(1, imageSamplerDesc),
            magma::bindings::FragmentStageBinding(2, imageSamplerDesc)
        }));
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformTransforms);
        descriptorSet->update(1, diffuse->imageView, anisotropicSampler);
        descriptorSet->update(2, specular->imageView, anisotropicSampler);
    }

    void setupPipeline()
    {
        pipelineLayout.reset(new magma::PipelineLayout(descriptorSetLayout));
        wireframeDrawPipeline.reset(new magma::GraphicsPipeline(device, pipelineCache,
            {
                VertexShader(device, "transform.o"),
                FragmentShader(device, "envmap.o")
            },
            mesh->getVertexInput(),
            magma::states::triangleList,
            negateViewport ? magma::states::fillCullBackCW : magma::states::fillCullBackCCW,
            magma::states::dontMultisample,
            magma::states::depthLessOrEqual,
            magma::states::dontBlendWriteRGB,
            {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass));
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
    return std::unique_ptr<IApplication>(new CubeMapApp(entry));
}
