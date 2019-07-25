#include "../framework/vulkanApp.h"
#include "../framework/knotMesh.h"
#include "../framework/utilities.h"

#define CAPTION "13 - Specialization constants"

// Use Space to toggle between albedo and shading.
// Use 1, 2, 3 to change shading branch.
// Use mouse to rotate knot.
class SpecializationApp : public VulkanApp
{
    enum ShadingType
    {
        NORMAL = 0,
        DIFFUSE,
        SPECULAR,
        CELL,
        ALBEDO,
        MAX_SHADER_VARIANTS
    };

    struct Constants
    {
        // If the specialization constant is of type boolean,
        // size must be the byte size of VkBool32
        VkBool32 colorFill;
        int shadingType;
    };

    struct alignas(16) UniformBlock
    {
        rapid::matrix worldView;
        rapid::matrix worldViewProj;
        rapid::matrix normalMatrix;
    };

    std::unique_ptr<KnotMesh> mesh;
    std::shared_ptr<magma::ShaderModule> vertexShader;
    std::shared_ptr<magma::ShaderModule> fragmentShader;
    std::shared_ptr<magma::UniformBuffer<UniformBlock>> uniformBuffer;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::vector<std::shared_ptr<magma::GraphicsPipeline>> pipelines;
    std::vector<std::shared_ptr<magma::CommandBuffer>> commandBuffers[2];

    rapid::matrix view;
    rapid::matrix proj;
    bool negateViewport = false;
    ShadingType shadingType = ShadingType::NORMAL;
    bool colorFill = true; // Albedo
    int pipelineIndex = ShadingType::ALBEDO;

public:
    SpecializationApp(const AppEntry& entry):
        VulkanApp(entry, TEXT(CAPTION), 512, 512, true)
    {
        initialize();
        negateViewport = extensions->KHR_maintenance1 || extensions->AMD_negative_viewport_height;
        setupView();
        createMesh();
        loadShaders();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline(NORMAL);
        setupPipeline(DIFFUSE);
        setupPipeline(SPECULAR);
        setupPipeline(CELL);
        setupPipeline(ALBEDO);
        for (uint32_t i = 0; i < MAX_SHADER_VARIANTS; ++i)
        {
            recordCommandBuffer(FrontBuffer, i);
            recordCommandBuffer(BackBuffer, i);
        }
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        queue->submit(
            commandBuffers[bufferIndex][pipelineIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished,
            renderFinished,
            waitFences[bufferIndex]);
    }

    virtual void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case '1': shadingType = ShadingType::NORMAL; break;
        case '2': shadingType = ShadingType::DIFFUSE; break;
        case '3': shadingType = ShadingType::SPECULAR; break;
        case '4': shadingType = ShadingType::CELL; break;
        case AppKey::Space: colorFill = !colorFill;
            break;
        }
        if (colorFill)
            pipelineIndex = ShadingType::ALBEDO;
        else
            pipelineIndex = shadingType;
        switch (pipelineIndex)
        {
        case ShadingType::NORMAL: setWindowCaption(TEXT(CAPTION" (Normals)")); break;
        case ShadingType::DIFFUSE: setWindowCaption(TEXT(CAPTION" (Diffuse)")); break;
        case ShadingType::SPECULAR: setWindowCaption(TEXT(CAPTION" (Specular)")); break;
        case ShadingType::CELL: setWindowCaption(TEXT(CAPTION" (Cell-shading)")); break;
        case ShadingType::ALBEDO: setWindowCaption(TEXT(CAPTION" (Albedo)")); break;
        }
        VulkanApp::onKeyDown(key, repeat, flags);
    }

    virtual void createCommandBuffers() override
    {
        VulkanApp::createCommandBuffers();
        commandBuffers[FrontBuffer] = commandPools[0]->allocateCommandBuffers(MAX_SHADER_VARIANTS, true);
        commandBuffers[BackBuffer] = commandPools[0]->allocateCommandBuffers(MAX_SHADER_VARIANTS, true);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 5.f);
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
        const rapid::matrix world = pitch * yaw;
        magma::helpers::mapScoped<UniformBlock>(uniformBuffer, true, [this, &world](auto *block)
        {
            block->worldView = world * view;
            block->worldViewProj = world * view * proj;
            block->normalMatrix = rapid::transpose(rapid::inverse(block->worldView));
        });
    }

    void createMesh()
    {
        constexpr uint32_t slices = 32;
        constexpr uint32_t stacks = 256;
        constexpr float radius = 0.25f;
        mesh = std::make_unique<KnotMesh>(3, slices, stacks, radius, false, cmdBufferCopy);
    }

    void loadShaders()
    {
        utilities::aligned_vector<char> bytecode;
        bytecode = utilities::loadBinaryFile("transform.o");
        vertexShader = std::make_shared<magma::ShaderModule>(device, (const uint32_t *)bytecode.data(), bytecode.size());
        bytecode = utilities::loadBinaryFile("specialized.o");
        fragmentShader = std::make_shared<magma::ShaderModule>(device, (const uint32_t *)bytecode.data(), bytecode.size());
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<UniformBlock>>(device);
    }

    void setupDescriptorSet()
    {
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, 1,
            std::vector<magma::Descriptor>
            {
                uniformBufferDesc
            });
        descriptorSetLayout = std::make_shared<magma::DescriptorSetLayout>(device,
            magma::bindings::VertexStageBinding(0, uniformBufferDesc));
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformBuffer);

        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
    }

    magma::FragmentShaderStage specializeFragmentStage(ShadingType shadingType, const char *entrypoint) const
    {
        Constants constants;
        constants.colorFill = MAGMA_BOOLEAN(ShadingType::ALBEDO == shadingType);
        constants.shadingType = static_cast<int>(shadingType);
        std::shared_ptr<magma::Specialization> specialization(std::make_shared<magma::Specialization>(constants,
            std::initializer_list<magma::SpecializationEntry>
            {
                magma::SpecializationEntry(0, &Constants::colorFill),
                magma::SpecializationEntry(1, &Constants::shadingType)
            }));
        return magma::FragmentShaderStage(fragmentShader, entrypoint, specialization);
    }

    void setupPipeline(ShadingType shadingType)
    {
        const std::vector<magma::PipelineShaderStage> shaderStages = {
            magma::VertexShaderStage(vertexShader, "main"),
            specializeFragmentStage(shadingType, "main")
        };
        auto pipeline(std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            shaderStages,
            mesh->getVertexInput(),
            magma::renderstates::triangleList,
            negateViewport ? magma::renderstates::fillCullBackCW : magma::renderstates::fillCullBackCCW,
            magma::renderstates::noMultisample,
            magma::renderstates::depthLessOrEqual,
            magma::renderstates::dontBlendWriteRgb,
            std::initializer_list<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass));
        pipelines.push_back(pipeline);
    }

    void recordCommandBuffer(uint32_t bufferIndex, uint32_t pipelineIndex)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[bufferIndex][pipelineIndex];
        cmdBuffer->begin();
        {
            cmdBuffer->setRenderArea(0, 0, width, height);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[bufferIndex],
                {
                    magma::clears::grayColor,
                    magma::clears::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(pipelineLayout, descriptorSet);
                cmdBuffer->bindPipeline(pipelines[pipelineIndex]);
                mesh->draw(cmdBuffer);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::make_unique<SpecializationApp>(entry);
}
